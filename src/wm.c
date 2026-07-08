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

int initialize_state(AppState *state) {
  IVirtualDesktopManager *desktop_manager = NULL;
  HRESULT hr =
      CoCreateInstance(&CLSID_VirtualDesktopManager, NULL, CLSCTX_INPROC_SERVER,
                       &IID_IVirtualDesktopManager, (void **)&desktop_manager);

  if (!SUCCEEDED(hr)) {
    printf("Failed to create IVirtualDesktopManager instance. Error: 0x%08lX\n",
           hr);
    return 1;
  }

  HWNDTemp hwnd_buffer = {NULL, 0, 20};
  hwnd_buffer.arr = (HWND *)malloc(sizeof(HWND) * hwnd_buffer.capacity);
  if (hwnd_buffer.arr == NULL)
    return 1;

  EnumWindows(enum_callback, (LPARAM)&hwnd_buffer);

  if (hwnd_buffer.counter > 0) {
    set_screen_dimensions(state, hwnd_buffer.arr[0]);
  }

  state->desktop_count = 0;
  state->desktop_active_index = -1;
  memset(state->desktop_list, 0, sizeof(state->desktop_list));

  state->desktop_manager = desktop_manager;

  track_desktops(state, &hwnd_buffer);

  state->desktop_active_index = -1;
  if (hwnd_buffer.counter > 0) {
    state->desktop_active_index = 0;
  }

  return 0;
}

void sort_linked_list(AppState *state) {
  // 1. Get a POINTER to the desktop so modifications persist
  VirtualDesktop *vd = &state->desktop_list[state->desktop_active_index];

  struct TrackedWindowNode *sorted = NULL;
  struct TrackedWindowNode *current_unsorted = vd->window_head;

  while (current_unsorted != NULL) {
    // Un-zoom windows if necessary
    if (IsZoomed(current_unsorted->data)) {
      ShowWindow(current_unsorted->data, SW_SHOWNORMAL);
    }

    struct TrackedWindowNode *next_unsorted = current_unsorted->next;

    int current_score = get_window_position_score(current_unsorted->data);

    struct TrackedWindowNode *prev = NULL;
    struct TrackedWindowNode *current_sorted = sorted;

    while (current_sorted != NULL &&
           get_window_position_score(current_sorted->data) < current_score) {
      prev = current_sorted;
      current_sorted = current_sorted->next;
    }

    if (prev == NULL) {
      // Inserting at the very beginning of the sorted list
      current_unsorted->next = sorted;
      sorted = current_unsorted;
    } else {
      // Inserting in the middle or at the end
      current_unsorted->next = current_sorted;
      prev->next = current_unsorted;
    }

    current_unsorted = next_unsorted;
  }
  vd->window_head = sorted;
}

// Fibonacci
int apply_fibonacci_layout(AppState *state) {
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
    printf("[START]: EVENT_SYSTEM_MINIMIZEEND|EVENT_OBJECT_SHOW\n");
    print_all_titles(GLOBAL_APP_STATE_PTR);
    if (GetParent(hwnd) == NULL && is_window_usable(hwnd)) {
      Sleep(20);

      GUID desktop_id;
      if (get_desktop_id(hwnd, &desktop_id,
                         GLOBAL_APP_STATE_PTR->desktop_manager)) {
        printf("Finding desktop error: %lu\n", GetLastError());
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
    printf("[END]: EVENT_SYSTEM_MINIMIZEEND|EVENT_OBJECT_SHOW\n");
    print_all_titles(GLOBAL_APP_STATE_PTR);
    break;
  }
  case EVENT_OBJECT_DESTROY:
  case EVENT_SYSTEM_MINIMIZESTART: {
    stop_tracking_window(GLOBAL_APP_STATE_PTR, hwnd);
    break;
  }
  case EVENT_SYSTEM_FOREGROUND: {
    GUID desktop_id;
    if (get_desktop_id(hwnd, &desktop_id,
                       GLOBAL_APP_STATE_PTR->desktop_manager) == 1)
      break;

    int found_index = find_tracked_desktop(GLOBAL_APP_STATE_PTR, &desktop_id);
    if (found_index == -1)
      break;

    GLOBAL_APP_STATE_PTR->desktop_active_index = found_index;
    break;
  }
  }
}

void change_desktop_focus(AppState *state, int desktop_index) {
  if (state->desktop_count <= desktop_index ||
      state->desktop_list[desktop_index].window_head == NULL) {
      // update this to debug
    return;
  }

  HWND hwnd = state->desktop_list[desktop_index].window_head->data;

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

BOOL CALLBACK enum_callback(HWND hwnd, LPARAM lparam) {
  HWNDTemp *temp = (HWNDTemp *)lparam;

  if (!is_window_usable(hwnd))
    return TRUE;

  // OWNER CHECK: Skip child windows or helper worker utility panels
  if (GetWindow(hwnd, GW_OWNER) != NULL)
    return TRUE;

  if (should_exclude(hwnd))
    return TRUE;

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

int get_window_title(HWND hwnd, char *buff, int max) {
  int len = GetWindowTextA(hwnd, buff, max);
  if (len == 0) {
    return 1;
  }
  return 0;
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
