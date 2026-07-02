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

int start_tracking_window(AppState *state, HWND hwnd) {
  struct TrackedWindowNode *newNode =
      (struct TrackedWindowNode *)malloc(sizeof(struct TrackedWindowNode));
  if (newNode == NULL) {
    return 1;
  }

  newNode->data = hwnd;
  newNode->next = NULL;

  if (state->window_ll == NULL) {
    state->window_ll = newNode;
    state->window_count++;
    return 0;
  }

  struct TrackedWindowNode *current = state->window_ll;
  while (current->next != NULL) {
    current = current->next;
  }

  current->next = newNode;
  state->window_count++;
  return 0;
}

int stop_tracking_window(AppState *state, HWND hwnd) {
  if (state->window_ll == NULL)
    return -1;

  struct TrackedWindowNode *tmp;
  if (state->window_ll->data == hwnd) {
    tmp = state->window_ll;
    state->window_ll = state->window_ll->next;
    free(tmp);
    state->window_count--;
    return 0;
  }

  struct TrackedWindowNode *current = state->window_ll;

  while (current->next != NULL) {
    if (current->next->data == hwnd) {
      tmp = current->next;
      current->next = current->next->next;
      free(tmp);
      return 0;
    }
    current = current->next;
  }

  return -1;
}

int reset_all_tracking_window(AppState *state) {
  if (state->window_ll == NULL)
    return 1;
  struct TrackedWindowNode *current = state->window_ll;
  struct TrackedWindowNode *next_node = NULL;

  while (current != NULL) {
    next_node = current->next; // Keep track of the next node
    free(current);             // Free the current node
    current = next_node;       // Move to the next node
  }

  state->window_ll = NULL; // Reset the pointer in your state struct
  state->window_count = 0;
  return 0;
}

int append_unique_desktop(AppState *state, GUID *desktop_id) {
  for (int i = 0; i < state->desktop_count; i++) {
    if (IsEqualGUID(&state->desktop_list[i], desktop_id)) {
      return 0;
    }
  }

  if (state->desktop_count >= state->desktop_capacity) {
    state->desktop_capacity *= 2;

    GUID *temp = (GUID *)realloc(state->desktop_list,
                                 sizeof(GUID) * state->desktop_capacity);
    if (temp == NULL)
      return 1;

    state->desktop_list = temp;
  }

  state->desktop_list[state->desktop_count] = *desktop_id;
  state->desktop_count++;
  return 0;
}

int focus_to_title(AppState *state, char *title) {
  struct TrackedWindowNode *current = state->window_ll;
  int cap = 255;
  char buff[cap];

  while (current != NULL) {
    int is_ok = get_window_title(current->data, buff, cap);
    printf("Comparing %s to %s\n", buff, title);
    if (is_ok == 0 && StrStrIA(buff, title) != NULL) {
      break;
    }
    current = current->next;
  }

  if (current == NULL)
    return 1;

  change_focus(current->data);

  return 0;
}
