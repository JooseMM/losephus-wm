#include <windows.h>

typedef struct {
  HWND *window_list;
  int window_ctr;
  int windows_cap;

  GUID *desktop_list;
  int desktop_ctr;
  int desktop_cap;

  int screen_x;
  int screen_y;
  int gap;
} AppState;

int initialize_state(AppState *state);

int print_window_title(HWND *hwnd);
