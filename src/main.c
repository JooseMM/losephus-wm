#include "wm.h"

#include <dwmapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <imm.h>
#include <processthreadsapi.h>
#include <stdio.h>
#include <synchapi.h>
#include <windef.h>
#include <windows.h>

static AppState *g_AppState = NULL;

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

  g_AppState = &state;

  // 1. Register your Hotkey
  if (register) {
    fprintf(stderr, "[Main] Hotkey registration failed. %lu\n", GetLastError());
    CoUninitialize();
    return 1;
  }

  // 2. Register for Windows Events (e.g., listening for foreground window
  // changes)
  HWINEVENTHOOK hEventHook = SetWinEventHook(
      EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_DESTROY, // Event range
      NULL,           // Handle to DLL (NULL for out-of-context)
      win_event_proc, // Your callback function
      0, 0,           // Process ID and Thread ID (0 = all)
      WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

  // FIX 2: Check if the hook actually succeeded
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
        // insertion_sort_list(&args->state->window_ll);
        // layout_fibonacci(args->state);
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
        break;

      case WM_ACTION_RESET_STATE:
        // reset_all_tracking_window(args->state);
        // EnumWindows(enum_callback, (LPARAM)args->state);
        break;

      case WM_ACTION_MOVE_UP: {
        // HWND target = GetForegroundWindow();
        // if (target != NULL) {
        //   change_window_position(args->state->window_ll, target, -1);
        //   layout_fibonacci(args->state);
        // }
        break;
      }
      case WM_ACTION_MOVE_DOWN: {
        HWND target = GetForegroundWindow();
        if (target != NULL) {
          // change_window_position(args->state->window_ll, target, 1);
          // layout_fibonacci(args->state);
        }
        break;
      }
      }
    // Windows Events are dispatched to WinEventProc here automatically
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // Cleanup
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
  if (!RegisterHotKey(NULL, WM_ACTION_RESET_STATE, MOD_ALT, 0x52))
    return 1;

  // Window Reposition
  if (!RegisterHotKey(NULL, WM_ACTION_MOVE_UP, MOD_ALT | MOD_SHIFT, 0x4B))
    return 1;
  if (!RegisterHotKey(NULL, WM_ACTION_MOVE_DOWN, MOD_ALT | MOD_SHIFT, 0x4A))
    return 1;

  return 0;
}

int unregister_hotkeys() {
  UnregisterHotKey(NULL, WM_ACTION_ORGANIZE);
  UnregisterHotKey(NULL, WM_ACTION_QUIT);
  UnregisterHotKey(NULL, WM_ACTION_KILL_WINDOW);
  UnregisterHotKey(NULL, WM_ACTION_MOVE_DOWN);
  UnregisterHotKey(NULL, WM_ACTION_MOVE_UP);
  UnregisterHotKey(NULL, WM_ACTION_RESET_STATE);
  UnregisterHotKey(NULL, WM_ACTION_OPEN_TERMINAL);

  return 0;
}
