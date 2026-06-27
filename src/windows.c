#include <stdio.h>
#include <stdlib.h>

#include "minwindef.h"
#include "windef.h"
#include "windows.h"
#include "winnt.h"
#include "wm.h"
#include <dwmapi.h>
#include <windows.h>

int is_window_visible(HWND hwnd);
BOOL CALLBACK enum_callback(HWND hwnd, LPARAM lparam);

int is_window_visible(HWND hwnd) {
  if (!IsWindowVisible(hwnd))
    return 0;

  if (GetWindow(hwnd, GW_OWNER) != NULL)
    return 0;

  if (GetWindowTextLengthW(hwnd) == 0)
    return 0;

  /* Alt-Tab rules to know what to show  */
  LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
  int is_tool_window = (ex_style & WS_EX_TOOLWINDOW) != 0;
  int is_app_window = (ex_style & WS_EX_APPWINDOW) != 0;
  if (is_tool_window && !is_app_window)
    return 0;

  /*
   * This instantly catches apps on other Virtual Desktops, suspended UWP apps
   * and hidden system panels (like the Windows 11 Quick Settings overlay). */
  int cloaked = 0;
  HRESULT hr =
      DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
  if (SUCCEEDED(hr) && cloaked != 0)
    return 0;

  return 1;
}

BOOL CALLBACK enum_callback(HWND hwnd, LPARAM lparam) {
  AppState *state = (AppState *)lparam;
  if (!is_window_visible(hwnd))
    return TRUE;

  if (state->window_counter >= state->windows_cap) {
    size_t new_cap = state->windows_cap * 2;

    HWND *new_list =
        (HWND *)realloc(state->window_list, new_cap * sizeof(HWND));
    if (!new_list)
      return FALSE;

    state->window_list = new_list;
    state->windows_cap = new_cap;
  }

  state->window_list[state->window_counter] = hwnd;
  state->window_counter++;
  return TRUE;
}

int initialize_dimensions(AppState *state) {
  MONITORINFO monitorInfo;
  monitorInfo.cbSize = sizeof(MONITORINFO);

  HMONITOR hMonitor =
      MonitorFromWindow(state->window_list[0], MONITOR_DEFAULTTONEAREST);
  GetMonitorInfo(hMonitor, &monitorInfo);

  // This RECT gives you the screen coordinates minus the Taskbar
  RECT workArea = monitorInfo.rcWork;

  state->screen_width = workArea.right - workArea.left;
  state->screen_height = workArea.bottom - workArea.top;
  state->gap = 10.0f;
  return 0;
}

int initialize_state(AppState *state) {
  initialize_dimensions(state);

  state->windows_cap = 2;
  state->window_counter = 0;
  HWND *new_list = malloc(state->windows_cap * sizeof(HWND));
  if (!new_list)
    return 1;

  state->window_list = new_list;
  EnumWindows(enum_callback, (LPARAM)state);
  return 0;
}

int print_window_title(HWND *hwnd) {
  WCHAR buff[255];
  int len = GetWindowTextW(*hwnd, buff, 255);
  if (len == 0) {
    printf("No Title.");
    return 1;
  }

  printf("Title: %ls\n", buff);
  return 0;
}

int organize_windows(AppState *state) {
if (state->window_counter == 0) return 0;

    for (int i = 0; i < state->window_counter; i++) {
        if (IsZoomed(state->window_list[i])) {
            ShowWindow(state->window_list[i], SW_RESTORE);
        }
    }

    const float gr = 1.618f;

    // Start coordinates fill 100% of the available workspace
    float rx = 0.0f + state->gap;
    float ry = 0.0f + state->gap;
    float rw = (float)state->screen_width - (state->gap * 2.0f);
    float rh = (float)state->screen_height - (state->gap * 2.0f);

    for (int i = 0; i < state->window_counter; i++) {
        HWND target = state->window_list[i];

        float win_x = rx;
        float win_y = ry;
        float win_w = rw;
        float win_h = rh;

        if (i < state->window_counter - 1) {
            if (i % 2 == 0) {
                // Vertical Split: Cut width by the golden ratio
                win_w = rw / gr;
                
                // Next origin starts exactly where this window ends
                rx += win_w;
                rw -= win_w;

		win_w -= state->gap;
            } 
            else {
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
        if (SUCCEEDED(DwmGetWindowAttribute(target, DWMWA_EXTENDED_FRAME_BOUNDS, &visual_rect, sizeof(RECT)))) {
            int left_padding   = visual_rect.left - real_rect.left;
            int right_padding  = real_rect.right - visual_rect.right;
            int bottom_padding = real_rect.bottom - visual_rect.bottom;

            int final_x = (int)win_x - left_padding;
            int final_y = (int)win_y;
            int final_w = (int)win_w + left_padding + right_padding;
            int final_h = (int)win_h + bottom_padding;

            SetWindowPos(target, NULL, final_x, final_y, final_w, final_h, SWP_SHOWWINDOW | SWP_NOZORDER);
        } else {
            SetWindowPos(target, NULL, (int)win_x, (int)win_y, (int)win_w, (int)win_h, SWP_SHOWWINDOW | SWP_NOZORDER);
        }
    }

    return 0;
}
