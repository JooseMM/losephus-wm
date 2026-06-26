#include <stdio.h>
#include <stdlib.h>

#include "minwindef.h"
#include "windef.h"
#include "windows.h"
#include "winnt.h"
#include "wm.h"
#include <windows.h>
#include <dwmapi.h>

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

  if (state->window_ctr >= state->windows_cap) {
    size_t new_cap = state->windows_cap * 2;

    HWND *new_list =
        (HWND *)realloc(state->window_list, new_cap * sizeof(HWND));
    if (!new_list)
      return FALSE;

    state->window_list = new_list;
    state->windows_cap = new_cap;
  }

  state->window_list[state->window_ctr] = hwnd;
  state->window_ctr++;
  return TRUE;
}

void calculate_workspace_dimensions(int* w, int* h) {
}

int initialize_state(AppState *state) {
  state->screen_x = GetSystemMetrics(SM_CXSCREEN);
  state->screen_y = GetSystemMetrics(SM_CYSCREEN);
  state->gap = 20;

  state->windows_cap = 2;
  state->window_ctr = 0;
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
  HWND target = state->window_list[0];
  if (IsZoomed(target)) {
    ShowWindow(target, SW_RESTORE);
  }

  SetWindowPos(target, NULL, 0 + state->gap, 0 + state->gap,
               state->screen_x - state->gap, state->screen_y - state->gap,
               SWP_SHOWWINDOW);
}
