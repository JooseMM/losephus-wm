#include "wm.h"
#include <windef.h>
#include <winnt.h>

typedef struct {
  HWND *arr;
  int counter;
  int capacity;
} HWNDTemp;

int start_tracking_window(VirtualDesktop *vd, HWND hwnd);
int stop_tracking_window(AppState *state, HWND hwnd);
int append_unique_desktop(AppState *state, GUID *desktop_id);
int find_tracked_desktop(AppState *state, GUID *id);

int should_exclude(HWND hwnd);
int is_window_usable(HWND hwnd);
int get_window_position_score(HWND hwnd);
int get_desktop_id(HWND hwnd, GUID *buff, IVirtualDesktopManager *pDesktopManager);
int set_screen_dimensions(AppState *state, HWND hwnd);
void track_desktops(AppState *state, HWNDTemp *tmp);

