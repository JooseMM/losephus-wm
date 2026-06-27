#include <windows.h>

typedef struct {
  HWND *window_list;
  int window_counter;
  int windows_cap;

  GUID *desktop_list;
  int desktop_ctr;
  int desktop_cap;

  float screen_width;
  float screen_height;
  float gap;
} AppState;

int initialize_state(AppState *state);

int print_window_title(HWND *hwnd);

int organize_windows(AppState *state);
