#include "utils.h"
#include <combaseapi.h>
#include <initguid.h>
#include <shobjidl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unknwn.h>
#include <windef.h>
#include <winnt.h>

#include <shlwapi.h> // Make sure to link -lshlwapi in your Makefile

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

// int reset_all_tracking_window(AppState *state) {
//   for (int i = 0; i < state->desktop_count; i++) {
//     VirtualDesktop vd = state->desktop_list[i];
//
//     if (vd->window_head == NULL)
//       return 1;
//
//     struct TrackedWindowNode *current = vd->window_head;
//     struct TrackedWindowNode *next_node = NULL;
//
//     while (current != NULL) {
//       next_node = current->next; // Keep track of the next node
//       free(current);             // Free the current node
//       current = next_node;       // Move to the next node
//     }
//
//     vd->window_head = NULL; // Reset the pointer in your state struct
//     vd->window_count = 0;
//   }
//   return 0;
// }

// int focus_to_title(AppState *state, char *title) {
//   struct TrackedWindowNode *current = state->window_ll;
//   int cap = 255;
//   char buff[cap];
//
//   while (current != NULL) {
//     int is_ok = get_window_title(current->data, buff, cap);
//     printf("Comparing %s to %s\n", buff, title);
//     if (is_ok == 0 && StrStrIA(buff, title) != NULL) {
//       break;
//     }
//     current = current->next;
//   }
//
//   if (current == NULL)
//     return 1;
//
//   change_focus(current->data);
//
//   return 0;
// }
