#include <stdio.h>
#include <combaseapi.h>
#include <initguid.h>
#include <shobjidl.h>
#include <stdlib.h>
#include <unknwn.h>
#include <windef.h>
#include <winnt.h>
#include <shlwapi.h>

#include "utils.h"

#define EXCLUDE_LIST_COUNT 2

int start_tracking_window(VirtualDesktop *vd, HWND hwnd) {
  struct TrackedWindowNode *newNode =
      (struct TrackedWindowNode *)malloc(sizeof(struct TrackedWindowNode));
  if (newNode == NULL) {
    return 1;
  }

  newNode->data = hwnd;
  newNode->next = NULL;

  if (vd->window_head == NULL) {
    vd->window_head = newNode;
    vd->window_count++;
    return 0;
  }

  struct TrackedWindowNode *current = vd->window_head;
  while (current->next != NULL) {
    current = current->next;
  }

  current->next = newNode;
  vd->window_count++;
  return 0;
}

int find_tracked_desktop(AppState *state, GUID *id) {
  for (int i = 0; i < state->desktop_count; i++) {
    if (IsEqualGUID(id, &state->desktop_list[i]))
      return i;
  }
  return -1;
}

int stop_tracking_window(AppState *state, HWND hwnd) {
  if (state->desktop_count == 0)
    return -1;

  for (int i = 0; i < state->desktop_count; i++) {
    VirtualDesktop vd = state->desktop_list[i];

    if (vd.window_count == 0)
      continue;

    struct TrackedWindowNode *current = vd.window_head;
    if (vd.window_head->data == hwnd) {
      vd.window_head = vd.window_head->next;
      free(current);
      vd.window_count--;
      return 0;
    }

    while (current->next != NULL) {
      if (current->next->data == hwnd) {
        struct TrackedWindowNode *tmp = current->next;
        current->next = current->next->next;
        free(tmp);
        return 0;
      }
      current = current->next;
    }
  }

  return -1;
}

int reset_all(AppState *state) {
  for (int i = 0; i < state->desktop_count; i++) {
    VirtualDesktop vd = state->desktop_list[i];

    if (vd.window_head == NULL)
      return 1;

    struct TrackedWindowNode *current = vd.window_head;
    struct TrackedWindowNode *next_node = NULL;

    while (current != NULL) {
      next_node = current->next; // Keep track of the next node
      free(current);             // Free the current node
      current = next_node;       // Move to the next node
    }

    vd.window_head = NULL; // Reset the pointer in your state struct
    vd.window_count = 0;
    memset(&state->desktop_list[i], 0, sizeof(VirtualDesktop));
  }

  state->screen_width = 0;
  state->screen_height = 0;
  state->desktop_count = 0;

  return 0;
}

int should_exclude(HWND hwnd) {
 char exclude_list[EXCLUDE_LIST_COUNT][255] = { 
  "Picture in Picture",
  "Picture-in-Picture"
 };
 char buff[500];

 if(get_window_title(hwnd, buff, 500) == 1)
   return 1;

 for(int i = 0; i < EXCLUDE_LIST_COUNT; i++) {
  if(StrStrIA(exclude_list[i], buff) != NULL) return 1;
 }

 return 0;
}

int is_window_usable(HWND wtarget) {
  if (!IsWindowVisible(wtarget))
    return 0;

  if (GetWindowTextLengthW(wtarget) == 0)
    return 0;

  // DIMENSION CHECK: Filter out windows with no actual physical area
  RECT rect;
  if (GetWindowRect(wtarget, &rect)) {
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
      return 0; // It has a title, but it occupies no physical space
    }
  }

  // Style Restrictions: Get the standard window style flags
  LONG style = GetWindowLong(wtarget, GWL_STYLE);
  LONG_PTR ex_style = GetWindowLongPtrW(wtarget, GWL_EXSTYLE);

  // If it's a child window, it belongs inside an app container, don't tile it
  if (style & WS_CHILD)
    return 0;

  // Alt-Tab rules: Drop tool windows unless they explicitly want to be app
  // windows
  int is_tool_window = (ex_style & WS_EX_TOOLWINDOW) != 0;
  int is_app_window = (ex_style & WS_EX_APPWINDOW) != 0;
  if (is_tool_window && !is_app_window)
    return 0;

  return 1;
}

int get_window_position_score(HWND hwnd) {
  RECT rect;
  if (GetWindowRect(hwnd, &rect)) {
    return rect.left + rect.top;
  } else {
    printf("Failed to get window position. Error: %lu\n", GetLastError());
    return 10000; // Fallback
  }
}

int get_desktop_id(HWND hwnd, GUID *buff,
                   IVirtualDesktopManager *pDesktopManager) {
  memset(buff, 0, sizeof(GUID));
  GUID desktop_id = {0};

  // Call GetWindowDesktopId using the C vtable structure
  HRESULT hr = pDesktopManager->lpVtbl->GetWindowDesktopId(pDesktopManager,
                                                           hwnd, &desktop_id);

  if (!SUCCEEDED(hr)) {
    // printf("Failed with HRESULT: 0x%08X\n", (unsigned int)hr);
    return 1;
  }

  if (IsEqualGUID(&desktop_id, &GUID_NULL)) {
    return 1;
  }

  *buff = desktop_id;
  return 0;
}

int set_screen_dimensions(AppState *state, HWND hwnd) {
  MONITORINFO monitorInfo;
  monitorInfo.cbSize = sizeof(MONITORINFO);

  HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  GetMonitorInfo(hMonitor, &monitorInfo);

  // This RECT gives you the screen coordinates minus the Taskbar
  RECT workArea = monitorInfo.rcWork;

  state->screen_width = workArea.right - workArea.left;
  state->screen_height = workArea.bottom - workArea.top;

  UINT dpi = GetDpiForWindow(hwnd);
  float scale_factor = (float)dpi / 96.0f;
  state->gap = 10.0f * scale_factor;
  return 0;
}

void track_desktops(AppState *state, HWNDTemp *tmp) {
  for (int i = 0; i < tmp->counter; i++) {
    GUID buff;
    int found_id = get_desktop_id(tmp->arr[i], &buff, state->desktop_manager);
    if (found_id == 1)
      continue;

    int index_of = -1;
    for (int x = 0; x < state->desktop_count; x++) {
      if (IsEqualGUID(&buff, &state->desktop_list[x])) {
        index_of = x;
        break;
      }
    }

    if (index_of != -1) {
      if (start_tracking_window(&state->desktop_list[index_of], tmp->arr[i]) ==
          1) {
        printf("OS is unable to give the necessary memory for allocation\n");
      }
    } else if (index_of == -1 &&
               state->desktop_count < state->desktop_capacity) {
      state->desktop_list[state->desktop_count] =
          (VirtualDesktop){buff, NULL, 0};
      ;

      if (start_tracking_window(&state->desktop_list[state->desktop_count],
                                tmp->arr[i]) == 1) {
        printf("OS is unable to give the necessary memory for allocation\n");
      }

      state->desktop_count++;
    }
  }
}
