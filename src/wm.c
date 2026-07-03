#include <errhandlingapi.h>
#include <handleapi.h>
#include <malloc.h>
#include <minwinbase.h>
#include <processthreadsapi.h>
#include <stdio.h>

#include <dwmapi.h>
#include <imm.h>
#include <minwindef.h>
#include <stdlib.h>
#include <windef.h>
#include <windows.h>
#include <winnt.h>

#include <combaseapi.h>
#include <initguid.h>
#include <shobjidl.h>

#include "utils.h"

typedef struct {
  HWND *arr;
  int counter;
  int capacity;
} HWNDTemp;

int is_window_usable(HWND hwnd);

int get_window_position_score(HWND hwnd);

int get_desktop_id(HWND hwnd, GUID *buff,
                   IVirtualDesktopManager *pDesktopManager);

int initialize_dimensions(AppState *state, HWND hwnd);

void track_uniques_desktops(AppState *state, HWNDTemp *tmp);

int initialize_state(AppState *state) {
  HWNDTemp hwnd_buffer = {NULL, 0, 20};

  hwnd_buffer.arr = (HWND *)malloc(sizeof(HWND) * hwnd_buffer.capacity);
  if (hwnd_buffer.arr == NULL)
    return 1;

  EnumWindows(enum_callback, (LPARAM)&hwnd_buffer);

  if (hwnd_buffer.counter > 0) {
    initialize_dimensions(state, hwnd_buffer.arr[0]);
  }

  state->desktop_count = 0;
  state->desktop_capacity = 9;
  state->desktop_active_index = -1;
  memset(state->desktop_list, 0, sizeof(state->desktop_list));

  // Create an instance of the VirtualDesktopManager
  IVirtualDesktopManager *desktop_manager = NULL;
  HRESULT hr =
      CoCreateInstance(&CLSID_VirtualDesktopManager, NULL, CLSCTX_INPROC_SERVER,
                       &IID_IVirtualDesktopManager, (void **)&desktop_manager);

  if (!SUCCEEDED(hr)) {
    printf("Failed to create IVirtualDesktopManager instance. Error: 0x%08lX\n",
           hr);
    return 1;
  }

  state->desktop_manager = desktop_manager;

  track_uniques_desktops(state, &hwnd_buffer);

  if (hwnd_buffer.counter > 0) {
    state->desktop_active_index = 0;
  }

  return 0;
}

void sorted_insert(struct TrackedWindowNode **sorted_head_ref,
                   struct TrackedWindowNode *new_node) {
  int new_node_score = get_window_position_score(new_node->data);

  if (*sorted_head_ref == NULL ||
      get_window_position_score((*sorted_head_ref)->data) >= new_node_score) {
    new_node->next = *sorted_head_ref;
    *sorted_head_ref = new_node;
    return;
  }

  struct TrackedWindowNode *current = *sorted_head_ref;
  while (current->next != NULL &&
         get_window_position_score(current->next->data) < new_node_score) {
    current = current->next;
  }

  new_node->next = current->next;
  current->next = new_node;
}

void insertion_sort_list(struct TrackedWindowNode **head_ref) {
  struct TrackedWindowNode *sorted = NULL;

  struct TrackedWindowNode *current = *head_ref;
  while (current != NULL) {
    struct TrackedWindowNode *next_node = current->next;

    if (IsZoomed(current->data)) {
      ShowWindow(current->data, SW_SHOWNORMAL);
    }

    sorted_insert(&sorted, current);
    current = next_node;
  }

  *head_ref = sorted;
}

int layout_fibonacci(AppState *state) {
  if (state->desktop_count == 0 || state->desktop_active_index == -1)
    return 0;

  const float gr = 1.618f;

  float rx = 0.0f + state->gap;
  float ry = 0.0f + state->gap;
  float rw = (float)state->screen_width - (state->gap * 2.0f);
  float rh = (float)state->screen_height - (state->gap * 2.0f);

  struct TrackedWindowNode *current =
      state->desktop_list[state->desktop_active_index].window_head;

  int counter = 0;
  while (current != NULL) {
    HWND target = current->data;

    float win_x = rx;
    float win_y = ry;
    float win_w = rw;
    float win_h = rh;

    if (current->next != NULL) {
      if (counter % 2 == 0) {
        // Vertical Split: Cut width by the golden ratio
        win_w = rw / gr;

        // Next origin starts exactly where this window ends
        rx += win_w;
        rw -= win_w;

        win_w -= state->gap;
      } else {
        // Horizontal Split: Cut height by the golden ratio
        win_h = rh / gr;

        // Next origin starts exactly where this window ends
        ry += win_h;
        rh -= win_h;

        win_h -= state->gap;
      }
    }

    // Apply DWM invisible border compensation so flush windows look correct
    RECT real_rect, visual_rect;
    GetWindowRect(target, &real_rect);
    if (SUCCEEDED(DwmGetWindowAttribute(target, DWMWA_EXTENDED_FRAME_BOUNDS,
                                        &visual_rect, sizeof(RECT)))) {
      int left_padding = visual_rect.left - real_rect.left;
      int right_padding = real_rect.right - visual_rect.right;
      int bottom_padding = real_rect.bottom - visual_rect.bottom;

      int final_x = (int)win_x - left_padding;
      int final_y = (int)win_y;
      int final_w = (int)win_w + left_padding + right_padding;
      int final_h = (int)win_h + bottom_padding;

      SetWindowPos(target, NULL, final_x, final_y, final_w, final_h,
                   SWP_SHOWWINDOW | SWP_NOZORDER);
    } else {
      SetWindowPos(target, NULL, (int)win_x, (int)win_y, (int)win_w, (int)win_h,
                   SWP_SHOWWINDOW | SWP_NOZORDER);
    }

    current = current->next;
    counter++;
  }
  return 0;
}

void CALLBACK win_event_proc(
    __attribute__((unused)) HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild, __attribute__((unused)) DWORD dwEventThread,
    __attribute__((unused)) DWORD dwmsEventTime) {
  // Filter out non-window objects (like controls, menus, carets, etc.)
  if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF) {
    return;
  }

  // Ensure the HWND is valid
  if (!hwnd) {
    return;
  }

  switch (event) {
  case EVENT_OBJECT_SHOW:
  case EVENT_SYSTEM_MINIMIZEEND: {
    if (GetParent(hwnd) == NULL && is_window_usable(hwnd)) {
      Sleep(20); // sleep to allow the OS to catch up
      GUID desktop_id;
      if (get_desktop_id(hwnd, &desktop_id,
                         GLOBAL_APP_STATE_PTR->desktop_manager) == 1) {
        break;
      }
      int found_index = find_tracked_desktop(GLOBAL_APP_STATE_PTR, &desktop_id);
      if (found_index == -1) {
        break;
      }

      if (start_tracking_window(
              &GLOBAL_APP_STATE_PTR->desktop_list[found_index], hwnd) == 1) {
        break;
      }
    }
    break;
  }
  case EVENT_OBJECT_DESTROY:
  case EVENT_SYSTEM_MINIMIZESTART: {
    stop_tracking_window(GLOBAL_APP_STATE_PTR, hwnd);
    break;
  }
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

void change_focus(HWND hwnd) {
  // Get the thread that currently "owns" the foreground
  HWND currentForeground = GetForegroundWindow();
  DWORD foregroundThreadId =
      currentForeground ? GetWindowThreadProcessId(currentForeground, NULL) : 0;

  // Get our own thread ID
  DWORD myThreadId = GetCurrentThreadId();

  // If we aren't the foreground thread, we must "attach" to it to steal focus
  // rights
  if (foregroundThreadId != myThreadId && foregroundThreadId != 0) {
    AttachThreadInput(foregroundThreadId, myThreadId, TRUE);

    // Bring window to top and force focus
    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    // Detach
    AttachThreadInput(foregroundThreadId, myThreadId, FALSE);
  } else {
    // We already have foreground rights
    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
  }
}

/* utils */
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

BOOL CALLBACK enum_callback(HWND hwnd, LPARAM lparam) {
  HWNDTemp *temp = (HWNDTemp *)lparam;

  if (!is_window_usable(hwnd))
    return TRUE;

  // OWNER CHECK: Skip child windows or helper worker utility panels
  if (GetWindow(hwnd, GW_OWNER) != NULL) {
    return TRUE;
  }

  if (temp->counter >= temp->capacity) {
    temp->capacity *= 2;
    HWND *tmp_realloc =
        (HWND *)realloc(temp->arr, sizeof(HWND) * temp->capacity);
    if (tmp_realloc == NULL)
      return FALSE;

    temp->arr = tmp_realloc;
  }

  temp->arr[temp->counter] = hwnd;
  temp->counter++;
  return TRUE;
}

int initialize_dimensions(AppState *state, HWND hwnd) {
  MONITORINFO monitorInfo;
  monitorInfo.cbSize = sizeof(MONITORINFO);

  HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  GetMonitorInfo(hMonitor, &monitorInfo);

  // This RECT gives you the screen coordinates minus the Taskbar
  RECT workArea = monitorInfo.rcWork;

  state->screen_width = workArea.right - workArea.left;
  state->screen_height = workArea.bottom - workArea.top;
  state->gap = 10.0f;
  return 0;
}

int get_window_title(HWND hwnd, char *buff, int max) {
  int len = GetWindowTextA(hwnd, buff, max);
  if (len == 0) {
    return 1;
  }
  return 0;
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

int open_terminal() {
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOW;

  wchar_t terminal[] = L"wt.exe";

  BOOL success = CreateProcessW(NULL, terminal, NULL, NULL, FALSE, 0, NULL,
                                NULL, &si, &pi);

  if (success) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  } else {
    printf("Failed to open wt.exe. Error: %lu\n", GetLastError());
  }
  return 0;
}

int change_window_position(struct TrackedWindowNode *head, HWND hwnd, int y) {
  if (y == 0)
    return 0;

  struct TrackedWindowNode *prev = NULL;
  struct TrackedWindowNode *current = head;

  while (current != NULL) {
    if (current->data == hwnd) {
      break;
    }

    prev = current;
    current = current->next;
  }

  if (current == NULL)
    return -1;

  if (y > 0 && current->next != NULL) {
    HWND tmp = current->data;
    current->data = current->next->data;
    current->next->data = tmp;
  }

  if (y < 0 && prev != NULL) {
    HWND tmp = current->data;
    current->data = prev->data;
    prev->data = tmp;
  }

  return 0;
}

void track_uniques_desktops(AppState *state, HWNDTemp *tmp) {
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
