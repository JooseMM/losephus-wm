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

AppState *GLOBAL_APP_STATE_PTR = NULL;

int main() {
  AppState state;
  if (initialize_state(&state) == 1) {
    fprintf(stderr, "[Main] Initialization error. %lu\n", GetLastError());
    return 1;
  }

  GLOBAL_APP_STATE_PTR = &state;
  printf("Current amount of desktop: %d", state.desktop_count);

  // Allocate safely on the HEAP
  HotkeyThreadArgs *thread_args =
      (HotkeyThreadArgs *)malloc(sizeof(HotkeyThreadArgs));
  if (thread_args == NULL) {
    fprintf(stderr, "[Main] Out of memory.\n");
    return 1;
  }
  thread_args->state = &state;
  thread_args->running = 1;

  // Hotkey worker thread
  HANDLE thread =
      CreateThread(NULL, 0, hotkey_tread_proc, thread_args, 0, NULL);
  if (thread == NULL) {
    free(thread_args);
    return 1;
  }

  // Window Listeners
  HWINEVENTHOOK hhook =
      // DESTROY: 0x8001 - SHOW: 0x8002 - MINIMIZE: 0x0016
      SetWinEventHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_OBJECT_SHOW, NULL,
                      win_event_proc, 0, 0, WINEVENT_OUTOFCONTEXT);
  if (!hhook) {
    DWORD error = GetLastError();
    fprintf(stderr, "Failed to register hook! Error code: %lu (0x%lX)\n", error,
            error);
    return 1;
  }

  printf("[Main] Listening for Window Creation/Destruction...\n");
  thread_args->main_thread_id = GetCurrentThreadId();
  MSG msg = {0};
  while (thread_args->running && GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // 4. Cleanup Sequence
  printf("[Main] Cleaning up components...\n");
  UnhookWinEvent(hhook);

  // Force background thread out of its blocking GetMessage
  PostThreadMessage(GetThreadId(thread), WM_QUIT, 0, 0);
  WaitForSingleObject(thread, INFINITE);
  CloseHandle(thread);
  CoUninitialize();

  free(thread_args);
  return 0;
}
