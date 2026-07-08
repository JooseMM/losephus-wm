#include "wm.h"
#include "utils.h"

#include <dwmapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <imm.h>
#include <processthreadsapi.h>
#include <stdio.h>
#include <synchapi.h>
#include <windef.h>
#include <windows.h>

AppState *GLOBAL_APP_STATE_PTR = NULL;

int main() {
  AppState state;

  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) {
    fprintf(stderr, "Failed to initialize COM.\n");
    return 1;
  }

  if (initialize_state(&state) == 1) {
    fprintf(stderr, "[Main] Initialization error. %lu\n", GetLastError());
    CoUninitialize();
    return 1;
  }

  GLOBAL_APP_STATE_PTR = &state;

  // 1. Register your Hotkey
  if (register_hotkeys() == 1) {
    fprintf(stderr, "[Main] Hotkey registration failed. %lu\n", GetLastError());
    CoUninitialize();
    return 1;
  }

  // Register for Windows Events
  HWINEVENTHOOK hEventHook = SetWinEventHook(
      EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_SHOW, NULL, win_event_proc, 0, 0,
      WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

  if (hEventHook == NULL) {
    fprintf(stderr, "[Main] WinEventHook registration failed. %lu\n",
            GetLastError());
    UnregisterHotKey(NULL, WM_ACTION_QUIT);
    CoUninitialize();
    return 1;
  }

  printf("Listening for Hotkeys and Window Events... Press Alt+Shift+Q to "
         "exit.\n");

  MSG msg = {0};
  while (GetMessage(&msg, NULL, 0, 0)) {
    switch (msg.wParam) {
    case WM_ACTION_ORGANIZE:
      sort_linked_list(&state);
      apply_fibonacci_layout(&state);
      break;
    case WM_ACTION_KILL_WINDOW:
      HWND current_focus = GetForegroundWindow();
      if (current_focus != NULL) {
        int res = PostMessage(current_focus, WM_CLOSE, 0, 0);
        if (!res) {
          fprintf(
              stderr,
              "Error when sending a close signal to the desire window:%lu\n",
              GetLastError());
        }
      }
      break;

    case WM_ACTION_QUIT:
      PostQuitMessage(0);
      break;

    case WM_ACTION_OPEN_TERMINAL:
      open_terminal();
      break;

    case WM_ACTION_MOVE_UP: {
      HWND target = GetForegroundWindow();
      if (target != NULL) {
        struct TrackedWindowNode *whead =
            state.desktop_list[state.desktop_active_index].window_head;
        change_window_position(whead, target, -1);
        apply_fibonacci_layout(&state);
      }
      break;
    }
    case WM_ACTION_MOVE_DOWN: {
      HWND target = GetForegroundWindow();
      if (target != NULL) {
        struct TrackedWindowNode *whead =
            state.desktop_list[state.desktop_active_index].window_head;
        change_window_position(whead, target, 1);
        apply_fibonacci_layout(&state);
      }
      break;
    }
    case WM_ACTION_FOCUS_DESKTOP_1: {
      if (state.desktop_count > 0) {
        change_focus(state.desktop_list[0].window_head->data);
      }
      break;
    }
    case WM_ACTION_FOCUS_DESKTOP_2: {
      if (state.desktop_count > 1) {
        change_focus(state.desktop_list[1].window_head->data);
      }
      break;
    }
    case WM_ACTION_FOCUS_DESKTOP_3: {
      if (state.desktop_count > 2)
        change_focus(state.desktop_list[2].window_head->data);
      break;
    }
    case WM_ACTION_FOCUS_DESKTOP_4: {
      if (state.desktop_count > 3)
        change_focus(state.desktop_list[3].window_head->data);
      break;
    }
    case WM_ACTION_FOCUS_DESKTOP_5: {
      if (state.desktop_count > 4)
        change_focus(state.desktop_list[4].window_head->data);
      break;
    }

    case WM_ACTION_FOCUS_DESKTOP_6: {
      if (state.desktop_count > 5)
        change_focus(state.desktop_list[5].window_head->data);
      break;
    }

    case WM_ACTION_FOCUS_DESKTOP_7: {
      if (state.desktop_count > 6)
        change_focus(state.desktop_list[6].window_head->data);
      break;
    }

    case WM_ACTION_FOCUS_DESKTOP_8: {
      if (state.desktop_count > 7)
        change_focus(state.desktop_list[7].window_head->data);
      break;
    }
    }
    // Windows Events are dispatched to WinEventProc here automatically
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // Cleanup
  state.desktop_manager->lpVtbl->Release(state.desktop_manager);
  unregister_hotkeys();
  UnhookWinEvent(hEventHook);
  UnregisterHotKey(NULL, WM_ACTION_QUIT);
  CoUninitialize();

  return 0;
}

int register_hotkeys() {
  if (!RegisterHotKey(NULL, WM_ACTION_ORGANIZE, MOD_ALT, 0x54))
    return 1;
  if (!RegisterHotKey(NULL, WM_ACTION_QUIT, MOD_ALT | MOD_SHIFT, 0x51))
    return 1;
  if (!RegisterHotKey(NULL, WM_ACTION_KILL_WINDOW, MOD_ALT, 0x51))
    return 1;
  if (!RegisterHotKey(NULL, WM_ACTION_OPEN_TERMINAL, MOD_ALT, 0x0D))
    return 1;

  // Window Reposition
  if (!RegisterHotKey(NULL, WM_ACTION_MOVE_UP, MOD_ALT | MOD_SHIFT, 0x4B))
    return 1;
  if (!RegisterHotKey(NULL, WM_ACTION_MOVE_DOWN, MOD_ALT | MOD_SHIFT, 0x4A))
    return 1;

  // Desktops
  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_1, MOD_ALT, 0x31))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_2, MOD_ALT, 0x32))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_3, MOD_ALT, 0x33))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_4, MOD_ALT, 0x34))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_5, MOD_ALT, 0x35))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_6, MOD_ALT, 0x36))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_7, MOD_ALT, 0x37))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_8, MOD_ALT, 0x38))
    return 1;

  if (!RegisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_9, MOD_ALT, 0x39))
    return 1;

  return 0;
}

void unregister_hotkeys() {
  UnregisterHotKey(NULL, WM_ACTION_ORGANIZE);
  UnregisterHotKey(NULL, WM_ACTION_QUIT);
  UnregisterHotKey(NULL, WM_ACTION_KILL_WINDOW);
  UnregisterHotKey(NULL, WM_ACTION_MOVE_DOWN);
  UnregisterHotKey(NULL, WM_ACTION_MOVE_UP);
  // UnregisterHotKey(NULL, WM_ACTION_RESET_STATE);
  UnregisterHotKey(NULL, WM_ACTION_OPEN_TERMINAL);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_1);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_2);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_3);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_4);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_5);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_6);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_7);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_8);
  UnregisterHotKey(NULL, WM_ACTION_FOCUS_DESKTOP_9);
}
