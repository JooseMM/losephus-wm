#include "windef.h"
#include <minwindef.h>
#include <shobjidl.h>
#include <windows.h>

struct TrackedWindowNode {
  HWND data;
  struct TrackedWindowNode *next;
};

typedef struct {
  GUID desktop_id;
  struct TrackedWindowNode *window_head;
  int window_count;
} VirtualDesktop;

typedef struct {
  IVirtualDesktopManager *desktop_manager;
  VirtualDesktop desktop_list[9];
  int desktop_count;
  int desktop_capacity;
  int desktop_active_index;

  float screen_width;
  float screen_height;
  float gap;

} AppState;

#ifndef WM_SHARED
#define WM_SHARED
extern AppState *GLOBAL_APP_STATE_PTR;
#endif

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

  WM_ACTION_FOCUS_DESKTOP_1,
  WM_ACTION_FOCUS_DESKTOP_2,
};

// 1. Structure to pass data safely to the background thread
typedef struct {
  AppState *state;
  int running;
  DWORD main_thread_id;
} HotkeyThreadArgs;

void sort_linked_list(struct TrackedWindowNode **head_ref);

DWORD WINAPI hotkey_tread_proc(LPVOID lpparam);
void CALLBACK win_event_proc(HWINEVENTHOOK hWinEventHook, DWORD event,
                             HWND hwnd, LONG idObject, LONG idChild,
                             DWORD dwEventThread, DWORD dwmsEventTime);

int initialize_state(AppState *state);

int get_window_title(HWND hwnd, char *buff, int max);

int apply_fibonacci_layout(AppState *state);

HWINEVENTHOOK register_focus_hook();

int change_window_position(struct TrackedWindowNode *head, HWND hwnd, int y);

int track_virtual_desktop(AppState *state, HWND hwnd);

int focus_to_title(AppState *state, char *title);

void change_focus(HWND hwnd);

int get_desktop_id(HWND hwnd, GUID *buff,
                   IVirtualDesktopManager *pDesktopManager);

int register_hotkeys();

void unregister_hotkeys();

int open_terminal();

BOOL CALLBACK enum_callback(HWND hwnd, LPARAM lparam);

int reset_all(AppState *state);
