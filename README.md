# Lightweight Integrated Desktop Environment

A lightweight, modular desktop environment written in C, featuring a custom window manager using Xlib and GTK3-based components. BlackLine (formerly LIDE) is designed for simplicity, performance, and a clean terminal-inspired aesthetic.

## Features

### Custom Window Manager (`blackline-wm`)
Xlib-based window manager with:
- Window decorations with custom title bars
- Draggable windows by title bar
- Keyboard shortcuts (Super+Enter, Super+Q, Super+Space)
- Proper window focus and stacking
- Client window closing support

### Top Panel (`blackline-panel`)
GTK3-based panel with:
- Dark theme (#0b0f14 background, #00ff88 accent)
- Live clock display with date
- "BlackLine" button (placeholder for future menu)
- "Tools" button to open or raise the tools container
- "Minimized Apps" button (📌) to toggle the minimized window list
- Full-width dock-style window that stays on top

### Application Launcher (`blackline-launcher`)
GTK3-based app launcher that:
- Reads .desktop files from `/usr/share/applications`
- Search-as-you-type functionality
- Dark themed interface matching the desktop
- Launches applications with a single click

### File Manager (`blackline-fm`)
Custom-built file manager with:
- Dual-pane interface (sidebar + main view)
- Navigation toolbar with back/forward/up/home/refresh
- Location bar for direct path entry
- Column view with Name, Size, Type, and Modified date
- Sortable columns (click headers to sort)
- Custom black background with red accents (#ff3333)
- Draggable window (click anywhere to drag)
- Minimize/maximize/close buttons in title bar
- Folder and file browsing with proper type detection
- Open files with default applications
- Edge-based window resizing with resize cursor hints

### Text Editor (`blackline-editor`)
Feature-rich text editor with:
- Full editing capabilities (cut, copy, paste, delete)
- Undo/redo functionality
- Find and replace
- Go to line
- Text transformations (uppercase, lowercase, capitalize)
- Line operations (comment, indent, unindent, duplicate, delete, join, sort)
- Print support
- Custom file dialogs with folder navigation
- Dark theme matching the desktop aesthetic
- Draggable window with minimize/maximize/close buttons
- Edge-based window resizing with resize cursor hints

### Terminal (`blackline-terminal`)
Tabbed terminal emulator using VTE:
- Multiple tabs with close buttons
- Scrollback support
- Custom title bar with minimize/maximize/close buttons
- Drag-to-move and edge-based resize with cursor hints

### Calculator (`blackline-calculator`)
Scientific calculator with:
- Basic arithmetic operations (+, -, ×, ÷)
- Advanced functions (sin, cos, tan, log, ln, sqrt, power)
- Memory functions (MC, MR, M+, MS)
- Parentheses support for complex expressions
- Keyboard input support
- Clean, intuitive interface
- Dark theme with green accent
- Edge-based window resizing with resize cursor hints

### System Monitor (`blackline-system-monitor`)
Real-time system monitoring with:
- CPU usage graph and percentage display
- Memory usage (total, used, free, cached, available)
- Process list with PID, name, CPU%, and memory%
- Auto-refresh every 2 seconds
- Visual graphs for CPU and memory trends
- Dark theme with green accents
- Draggable window with minimize/maximize/close buttons
- Edge-based window resizing with resize cursor hints

### Web Browser - VoidFox (`voidfox`)
Lightweight web browser built with WebKitGTK, featuring a full application menu and built-in management tabs:
- **Tabbed browsing** with multiple tabs and close buttons on each tab
- **Navigation controls** (back, forward, reload, stop)
- **Smart URL bar** – automatically detects if input is a URL or search term
- **Home button** with customizable home page
- **Search button** for quick web searches
- **Application menu** (☰) with:
  - New window / New private window
  - Bookmarks, History, Downloads, Passwords, Themes
  - Print, Find in page, Zoom controls, Settings, Report broken site
- **Bookmarks manager** – add, manage, and open bookmarks; persistent storage in `bookmarks.txt`
- **History viewer** – browse visited pages with timestamps; clear history
- **Downloads manager** – track download progress; open download folder; persistent storage in `downloads.txt`
- **Password manager** – store and manage site credentials; copy to clipboard; show/hide passwords; persistent storage in `passwords.txt`
- **Settings dialog** – home page, search engine, privacy, permissions, appearance, and download preferences; stored in `~/.config/lide/voidfox/voidfox_settings.txt`
- **Permission controls** – location, notifications, camera, and microphone gated by settings
- **Extensions placeholder** – future extension support
- Custom title bar with minimize/maximize/close buttons
- Dark theme with green accent (#00ff88)
- Draggable window (click anywhere on title bar to drag)
- Edge-based window resizing with resize cursor hints

### Firefox Launcher (`firefox-wrapper`)
GTK wrapper that launches system Firefox:
- Checks for `firefox` or `firefox-esr` availability
- Launch status feedback and duplicate-run protection
- Custom title bar with minimize/maximize/close buttons
- Draggable window

### Tools Container (`blackline-tools`)
Utility window providing quick access to all BlackLine tools:
- **File Manager** – Launches `blackline-fm`
- **Text Editor** – Launches `blackline-editor`
- **Terminal** – Launches `blackline-terminal`
- **Calculator** – Launches `blackline-calculator`
- **System Monitor** – Launches `blackline-system-monitor`
- **Web Browser** – Launches `voidfox`
- **Firefox** – Launches `firefox-wrapper`
- Clean, icon-based interface with emoji icons for easy recognition
- List/grid view toggle with persisted mode in `~/.config/blackline/tools_view_mode.conf`
- Fixed position below the panel, always on top
- Close button
- Dark theme matching the desktop aesthetic
- Automatically closes after launching an application

### Minimized Apps Container
Window list for minimized apps:
- Tracks minimized windows via X11 events
- Restore or close individual windows
- "Close All" option for bulk cleanup
- Toggle from the panel's 📌 button

### Wallpaper Service (`blackline-background`)
Continuous background setter that:
- Uses `feh` to set wallpaper from `./images/wal1.png`
- Falls back to solid color (#0b0f14) if wallpaper not found
- Runs continuously to ensure wallpaper persists
- Prevents window managers from overriding the background

### Session Manager (`blackline-session`)
Starts all components in the correct order:
- Wallpaper service → Panel → Window Manager
- Proper process management and cleanup on exit
- Handles SIGINT and SIGTERM gracefully

## Build & Run

### Build
- Install build deps: `gcc`, `make`, `pkg-config`, `gtk+-3.0`, `x11`
- VoidFox requires `webkit2gtk-4.1` or `webkit2gtk-4.0`
- Terminal requires `vte-2.91` (skipped if missing)
- Wallpaper uses `feh` (recommended)
- Build everything: `make` (terminal is skipped if `vte-2.91` is missing)
- Install system-wide: `make install` (uses `sudo` and copies to `/usr/local/bin`)

### Run (nested X test)
- `./run.sh` (uses `Xephyr`, `xdpyinfo`, and `pkill`)
- Note: `run.sh` assumes the repo is at `~/Desktop/LIDE`
- Requires `Xephyr` installed

### Run (manual)
- `./blackline-background &`
- `./blackline-panel &`
- `./blackline-wm &`
- Optional tools: `./blackline-tools` (or launch any component directly)

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Super+Enter | Launch terminal (xterm) |
| Super+Q | Close focused window |
| Super+Space | Open application launcher |

## Project Structure
LIDE/
├── wm/ # Window manager
│ ├── wm.c # Main WM event loop
│ ├── keybinds.c # Keyboard shortcut handling
│ ├── layout.c # Window layout and decorations
│ ├── wm_debug.c # Debugging utilities
│ └── wm.h # WM headers
│
├── panel/ # Top panel
│ ├── panel.c # Panel GUI and logic
│ ├── clock.c # Clock update function
│ ├── diagnose.sh # Panel diagnostic script
│ └── panel.h # Panel headers
│
├── launcher/ # Application launcher
│ ├── launcher.c # Launcher GUI and .desktop parsing
│ ├── search.c # Search functionality
│ └── launcher.h # Launcher headers
│
├── tools/ # Tools and utilities
│ ├── background.c # Continuous wallpaper service
│ ├── wallpaper.c # Wallpaper setter
│ ├── minimized_container.c # Minimized window container
│ ├── minimized_container.h # Container headers
│ ├── tools_container.c # Tools window
│ ├── viewMode.c # Tools view mode persistence
│ ├── viewMode.h # View mode headers
│ ├── window_resize.h # Shared edge-resize helpers
│ │
│ ├── file-manager/ # Custom file manager
│ │ ├── fm.c # File manager GUI
│ │ ├── browser.c # File browsing logic
│ │ └── fm.h # File manager headers
│ │
│ ├── text_editor/ # Text editor
│ │ ├── editor.c # Editor GUI
│ │ ├── edit.c # Editing functionality
│ │ ├── edit.h # Edit headers
│ │ └── text_editor.h # Editor headers
│ │
│ ├── terminal/ # Terminal
│ │ └── terminal.c # VTE-based tabbed terminal
│ │
│ ├── calculator/ # Calculator
│ │ ├── calculator.c # Calculator logic and GUI
│ │ └── calculator.h # Calculator headers
│ │
│ ├── system-monitor/ # System monitor
│ │ ├── monitor.c # Main monitor GUI
│ │ ├── cpu.c # CPU monitoring
│ │ ├── memory.c # Memory monitoring
│ │ ├── processes.c # Process list
│ │ └── monitor.h # Monitor headers
│ │
│ ├── firefox/ # Firefox launcher
│ │ ├── firefox-wrapper.c # System Firefox launcher UI
│ │ ├── firefox.c # Lightweight WebKit browser UI
│ │ ├── firefox.h # Firefox headers
│ │ └── firefox.desktop # Desktop entry
│ │
│ └── web-browser/ # VoidFox web browser
│ ├── voidfox.c # Main browser entry
│ ├── browser.c # Browser GUI and logic
│ ├── tab.c # Tab management
│ ├── tab.h # Tab headers
│ ├── app_menu.c # Application menu
│ ├── app_menu.h # Menu headers
│ ├── bookmarks.c # Bookmarks manager
│ ├── bookmarks.h # Bookmarks headers
│ ├── history.c # History manager
│ ├── history.h # History headers
│ ├── downloads.c # Downloads manager
│ ├── downloads.h # Downloads headers
│ ├── passwords.c # Password manager
│ ├── passwords.h # Password headers
│ ├── extensions.c # Extensions placeholder
│ ├── extensions.h # Extensions headers
│ ├── settings.c # Settings dialog and persistence
│ ├── settings.h # Settings headers
│ └── voidfox.h # Main browser headers
│
├── session/ # Session manager
│ └── session.c # Process launcher and manager
│
├── themes/ # Theme files
│ ├── lide.css # GTK CSS styling
│ └── Xresources # xterm configuration
│
├── images/ # Wallpaper images
│ ├── wal1.png # Default wallpaper
│ └── web.png # VoidFox background image
│
├── Downloads/ # Default download directory (created by VoidFox)
│
├── include/ # Header files (future use)
├── Makefile # Build configuration
├── run.sh # Development test script
├── blackline.desktop # Desktop entry for display managers
└── README.md # This file


## Building from Source

### Dependencies
 
 chmod +x ./deps.sh
 ./deps.sh
 #make sure u run it in root

Running BlackLine
# Run the test script
./run.sh

Theme

BlackLine features a terminal-inspired dark theme:

    Background: #0b0f14 (dark charcoal)

    Accent: #00ff88 (vibrant green)

    File Manager: Black background (#000000) with red accents (#ff3333)

    Text Editor: Black background with red accents

    Calculator: Dark theme with green buttons

    System Monitor: Dark theme with green graphs

    VoidFox Browser: Dark theme with green accents and custom logo, plus styled menu and tab close buttons

    Text: White (#ffffff) for readability

    Borders: Subtle accent-colored borders

Tools Overview
File Manager

    Browse files and folders with intuitive column view

    Sort by name, size, type, or date

    Navigate with toolbar buttons or keyboard

    Open files with default applications

Text Editor

    Full-featured editing with syntax highlighting

    Find and replace with options

    Line operations and text transformations

    Print support

Calculator

    Scientific calculator with memory functions

    Keyboard input support

    Expression evaluation with parentheses

System Monitor

    Real-time CPU and memory graphs

    Process list with sorting

    System resource overview

VoidFox Web Browser

    Tabbed browsing with close buttons on each tab

    Application menu for browser management

    Bookmarks, History, Downloads, Passwords tabs for easy data management

    Smart URL/search bar – type a URL or search term

    Persistent storage – bookmarks, history, downloads, passwords saved to disk

    Find in page with case‑sensitive option

    Print support via JavaScript window.print()

    Zoom controls (in/out/reset)

    Report broken site – search Google for the current URL with "report broken site"

Troubleshooting
Common Issues

Q: The wallpaper doesn't show up.
A: Ensure feh is installed and the image exists at ~/Desktop/LIDE/images/wal1.png.

Q: Keyboard shortcuts don't work.
A: Make sure the window manager is running and has focus. Check for conflicts with other applications.

Q: The file manager won't launch from Tools.
A: Verify blackline-fm was built successfully and is in the same directory as the other binaries.

Q: VoidFox browser crashes on startup.
A: Set environment variables: export WEBKIT_DISABLE_COMPOSITING_MODE=1 and export LIBGL_ALWAYS_SOFTWARE=1

Q: VoidFox shows a Google "sorry" page when searching.
A: This is a temporary block. The browser now sets a modern user agent; try again later or use a different search engine via Settings.

Q: Windows aren't draggable.
A: Click anywhere on the window (not just the title bar) to drag - this is by design.

Q: The calculator shows wrong results.
A: Ensure proper syntax: use * for multiplication, / for division, ^ for power.

Q: System Monitor shows no processes.
A: Check if /proc is accessible. Run with appropriate permissions.
Future Plans

    System settings panel

    Notification area

    Workspace switcher

    Application dock

    Network manager integration

    Audio controls

    Power management

    Additional desktop widgets

    Configuration file support

    Theme selector

Contributing

Contributions are welcome! Areas for improvement:

    Additional desktop tools

    Workspace support

    System tray

    Configuration file support

    Application menu

    Performance optimizations

    Bug fixes and stability improvements

License

This project is open source. Feel free to use, modify, and distribute it according to the license terms.
