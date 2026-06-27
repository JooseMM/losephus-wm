#include "wm.h"

int main() {
  AppState state;
  initialize_state(&state);
  organize_windows(&state);
  return 0;
}
