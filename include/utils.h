#include "wm.h"
#include <windef.h>
#include <winnt.h>

int start_tracking_window(AppState *state, HWND hwnd);
int stop_tracking_window(AppState *state, HWND hwnd);
int reset_all_tracking_window(AppState *state);

int append_unique_desktop(AppState *state, GUID *desktop_id);

