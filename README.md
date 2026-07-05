# Windows Tiling Window Manager (Losephus-WM)

A lightweight, tiling window manager for Windows written natively in C. Utilizing the native Win32 API, Windows Hooks (`SetWinEventHook`), and Component Object Model (COM) interfaces, this project brings dynamic, keyboard-driven layout management—similar to Unix tools like `bspwm` or `i3wm`—directly to Windows.

The core architecture tracks application environments across virtual desktops and systematically handles window placement using a deterministic **Fibonacci layout algorithm**.

---

## 🚀 Features

- **Dynamic Fibonacci Tiling:** re-arranges active application windows following a golden-ratio spiral configuration.
- **Native Virtual Desktop Integration:** Utilizes the Windows COM layer (`IVirtualDesktopManager` / `desktop_manager`) to separate layouts dynamically per active virtual workspace.
- **Asynchronous Global Hotkeys:** Low-overhead global keyboard shortcuts registered directly at the OS level thread queue.
- **Event-Driven Layout Auditing:** Hooks directly into Windows system messages (`EVENT_SYSTEM_MINIMIZESTART`, `EVENT_OBJECT_SHOW`, etc.) to intercept creation, destruction, or layout changes and re-calculate dimensions smoothly.
- **Terminal Optimization:** Seamlessly spins up shell processes natively via configured actions.

---

## ⌨️ Default Keybindings

The window manager exposes standard ergonomic shortcuts mapping the `Alt` (`MOD_ALT`) and `Shift` (`MOD_SHIFT`) modifiers to actions.

| Keybinding | Action | Win32 Identifier | Virtual Key Code |
| :--- | :--- | :--- | :--- |
| `Alt + T` | Re-organize/Sort Layout | `WM_ACTION_ORGANIZE` | `0x54` |
| `Alt + Enter` | Open Terminal | `WM_ACTION_OPEN_TERMINAL` | `0x0D` |
| `Alt + Q` | Kill Currently Focused Window | `WM_ACTION_KILL_WINDOW` | `0x51` |
| `Alt + Shift + K` | Shift Focused Window Up / Backward | `WM_ACTION_MOVE_UP` | `0x4B` |
| `Alt + Shift + J` | Shift Focused Window Down / Forward | `WM_ACTION_MOVE_DOWN` | `0x4A` |
| `Alt + 1` | Shift Focus to Desktop Workspace 1 | `WM_ACTION_FOCUS_DESKTOP_1` | `0x31` |
| `Alt + 2` | Shift Focus to Desktop Workspace 2 | `WM_ACTION_FOCUS_DESKTOP_2` | `0x32` |
| `Alt + Shift + Q` | **Exit & Gracefully Unhook Window Manager** | `WM_ACTION_QUIT` | `0x51` |

---

## 🛠️ Architecture Overview

The program initializes an internal state tracking context and hooks directly into the core operating system framework:

1. **COM Multi-Threading Initialization:** Invokes `CoInitializeEx` with an apartment-threaded structural model (`COINIT_APARTMENTTHREADED`) to instantiate shell integrations safely.
2. **Global Event Hooks:** Employs `SetWinEventHook` in an out-of-context configuration (`WINEVENT_OUTOFCONTEXT`) to receive non-blocking system messages safely without dll injection.
3. **Data Management:** Maintained layout contexts use a tracked linear linked-list (`TrackedWindowNode`) nested inside segmented array structures mirroring your OS virtual desktops.

---

## 📦 Compilation & Building

The easiest way to build the project is by using GNU `make` paired with a GCC configuration (like MinGW-w64) on Windows.

### ⚡ Quick Start: Build & Run
To compile your source files, link dependencies, and instantly spin up the window manager program, run:
```bash
make run
```
### Running the Application
```bash
./losephus.exe
```
