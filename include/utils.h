#include <windef.h>
#include <winnt.h>

struct TrackedWindowNode {
  HWND data;
  struct TrackedWindowNode *next;
};

int append_trackable_window(struct TrackedWindowNode **head_ref, HWND hwnd);
int remove_trackable_window(struct TrackedWindowNode **head_ref, HWND hwnd);
int reset_trackable_window(struct TrackedWindowNode **head_ref);

struct TrackedDesktopNode {
  GUID data;
  struct TrackedWindowNode *next;
};
