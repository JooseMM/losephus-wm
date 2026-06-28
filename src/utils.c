#include "utils.h"
#include <stdlib.h>
#include <windef.h>

int append_trackable_window(struct TrackedWindowNode **head_ref, HWND hwnd) {
  struct TrackedWindowNode *newNode =
      (struct TrackedWindowNode *)malloc(sizeof(struct TrackedWindowNode));
  if (newNode == NULL) {
    return 1;
  }

  newNode->data = hwnd;
  newNode->next = NULL;

  if (*head_ref == NULL) {
    *head_ref = newNode;
    return 0;
  }

  struct TrackedWindowNode *current = *head_ref;
  while (current->next != NULL) {
    current = current->next;
  }

  current->next = newNode;

  return 0;
}

int remove_trackable_window(struct TrackedWindowNode **head_ref, HWND hwnd) {
  if (*head_ref == NULL)
    return -1;

  struct TrackedWindowNode *tmp;
  if ((*head_ref)->data == hwnd) {
    tmp = *head_ref;
    *head_ref = (*head_ref)->next;
    free(tmp);
    return 0;
  }

  struct TrackedWindowNode *current = *head_ref;

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
