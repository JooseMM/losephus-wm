#include <initguid.h>
#include "utils.h"
#include "windows.h"
#include <combaseapi.h>
#include <shobjidl.h>
#include <unknwn.h>
#include <stdlib.h>
#include <windef.h>

wchar_t *get_desktop_name(HDESK hDesktop) {
  if (hDesktop == NULL)
    return NULL;

  DWORD needed_size = 0;

  // 1. Get the required buffer size
  GetUserObjectInformationW(hDesktop, UOI_NAME, NULL, 0, &needed_size);
  if (needed_size == 0)
    return NULL;

  // 2. Allocate the buffer
  wchar_t *name_buffer = (wchar_t *)malloc(needed_size);
  if (name_buffer == NULL)
    return NULL;

  // 3. Populate the buffer with the name
  if (!GetUserObjectInformationW(hDesktop, UOI_NAME, name_buffer, needed_size,
                                 &needed_size)) {
    free(name_buffer);
    return NULL;
  }

  return name_buffer;
}

int append_trackable_window(AppState *state, HWND hwnd) {
  struct TrackedWindowNode *newNode =
      (struct TrackedWindowNode *)malloc(sizeof(struct TrackedWindowNode));
  if (newNode == NULL) {
    return 1;
  }

  newNode->data = hwnd;
  newNode->next = NULL;

  if (state->window_ll == NULL) {
    state->window_ll = newNode;
    state->window_counter++;
    return 0;
  }

  struct TrackedWindowNode *current = state->window_ll;
  while (current->next != NULL) {
    current = current->next;
  }

  current->next = newNode;
  state->window_counter++;
  return 0;
}

int remove_trackable_window(AppState *state, HWND hwnd) {
  if (state->window_ll == NULL)
    return -1;

  struct TrackedWindowNode *tmp;
  if (state->window_ll->data == hwnd) {
    tmp = state->window_ll;
    state->window_ll = state->window_ll->next;
    free(tmp);
    state->window_counter--;
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

int reset_trackable_window(AppState *state) {
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
  state->window_counter = 0;
  return 0;
}
