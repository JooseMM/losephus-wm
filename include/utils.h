#include <windef.h>
#include <winnt.h>
#include "wm.h"

struct TrackedWindowNode {
  HWND data;
  struct TrackedWindowNode *next;
};

int append_trackable_window(AppState *state, HWND hwnd);
int remove_trackable_window(AppState *state, HWND hwnd);
int reset_trackable_window(AppState *state);

struct TrackedDesktopNode {
  GUID data;
  struct TrackedWindowNode *next;
};
