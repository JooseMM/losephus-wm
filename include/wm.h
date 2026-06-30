#include "windef.h"
#include <minwindef.h>
#include <windows.h>

enum {
  // Window management
  WM_ACTION_ORGANIZE = 1,
  WM_ACTION_QUIT,
  WM_ACTION_KILL_WINDOW,
  WM_ACTION_OPEN_TERMINAL,
  WM_ACTION_RESET_STATE,

  // Window Reposition
  WM_ACTION_MOVE_UP,
  WM_ACTION_MOVE_DOWN,
  WM_ACTION_MOVE_RIGHT,
  WM_ACTION_MOVE_LEFT,
};

typedef struct {
  struct TrackedWindowNode *window_ll;
  int window_counter;

  GUID *desktop_list;
  int desktop_count;
  int desktop_capacity;

  float screen_width;
  float screen_height;
  float gap;
} AppState;

// 1. Structure to pass data safely to the background thread
typedef struct {
  AppState *state;
  int running;
  DWORD main_thread_id;
} HotkeyThreadArgs;

extern AppState *GLOBAL_APP_STATE_PTR;

DWORD WINAPI hotkey_tread_proc(LPVOID lpparam);
void CALLBACK win_event_proc(HWINEVENTHOOK hWinEventHook, DWORD event,
                             HWND hwnd, LONG idObject, LONG idChild,
                             DWORD dwEventThread, DWORD dwmsEventTime);

int initialize_state(AppState *state);

int print_window_title(HWND hwnd);

int layout_fibonacci(AppState *state);

HWINEVENTHOOK register_focus_hook();

int change_window_position(struct TrackedWindowNode *head, HWND hwnd, int y);
