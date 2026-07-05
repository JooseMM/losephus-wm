#include "utils.h"
#include <combaseapi.h>
#include <initguid.h>
#include <shobjidl.h>
#include <stdlib.h>
#include <unknwn.h>
#include <windef.h>
#include <winnt.h>

#include <shlwapi.h>
#define EXCLUDE_LIST_COUNT 2

int start_tracking_window(VirtualDesktop *vd, HWND hwnd) {
  struct TrackedWindowNode *newNode =
      (struct TrackedWindowNode *)malloc(sizeof(struct TrackedWindowNode));
  if (newNode == NULL) {
    return 1;
  }

  newNode->data = hwnd;
  newNode->next = NULL;

  if (vd->window_head == NULL) {
    vd->window_head = newNode;
    vd->window_count++;
    return 0;
  }

  struct TrackedWindowNode *current = vd->window_head;
  while (current->next != NULL) {
    current = current->next;
  }

  current->next = newNode;
  vd->window_count++;
  return 0;
}

int find_tracked_desktop(AppState *state, GUID *id) {
  for (int i = 0; i < state->desktop_count; i++) {
    if (IsEqualGUID(id, &state->desktop_list[i]))
      return i;
  }
  return -1;
}

int stop_tracking_window(AppState *state, HWND hwnd) {
  if (state->desktop_count == 0)
    return -1;

  for (int i = 0; i < state->desktop_count; i++) {
    VirtualDesktop vd = state->desktop_list[i];

    if (vd.window_count == 0)
      continue;

    struct TrackedWindowNode *current = vd.window_head;
    if (vd.window_head->data == hwnd) {
      vd.window_head = vd.window_head->next;
      free(current);
      vd.window_count--;
      return 0;
    }

    while (current->next != NULL) {
      if (current->next->data == hwnd) {
        struct TrackedWindowNode *tmp = current->next;
        current->next = current->next->next;
        free(tmp);
        return 0;
      }
      current = current->next;
    }
  }

  return -1;
}

int reset_all(AppState *state) {
  for (int i = 0; i < state->desktop_count; i++) {
    VirtualDesktop vd = state->desktop_list[i];

    if (vd.window_head == NULL)
      return 1;

    struct TrackedWindowNode *current = vd.window_head;
    struct TrackedWindowNode *next_node = NULL;

    while (current != NULL) {
      next_node = current->next; // Keep track of the next node
      free(current);             // Free the current node
      current = next_node;       // Move to the next node
    }

    vd.window_head = NULL; // Reset the pointer in your state struct
    vd.window_count = 0;
    memset(&state->desktop_list[i], 0, sizeof(VirtualDesktop));
  }

  state->screen_width = 0;
  state->screen_height = 0;
  state->desktop_count = 0;

  return 0;
}

int should_exclude(HWND hwnd) {
 char exclude_list[EXCLUDE_LIST_COUNT][255] = { 
  "Picture in Picture",
  "Picture-in-Picture"
 };
 char buff[500];

 if(get_window_title(hwnd, buff, 500) == 1)
   return 1;

 for(int i = 0; i < EXCLUDE_LIST_COUNT; i++) {
  if(StrStrIA(exclude_list[i], buff) != NULL) return 1;
 }

 return 0;
}
