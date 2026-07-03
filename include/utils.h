#include "wm.h"
#include <windef.h>
#include <winnt.h>

int start_tracking_window(VirtualDesktop *vd, HWND hwnd);
int stop_tracking_window(AppState *state, HWND hwnd);

int append_unique_desktop(AppState *state, GUID *desktop_id);
int find_tracked_desktop(AppState *state, GUID *id);

