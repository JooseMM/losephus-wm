#include "wm.h"
#include <dwmapi.h>
#include <windef.h>
#include <windows.h>

// Subscribe to focus, creation, deletion, move
int main() {
  AppState state;
  initialize_state(&state);
  organize_windows(&state);
  return 0;
}
