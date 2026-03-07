# BlackLine Desktop Environment

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
- "Tools" button to open tools container
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
- Minimize and close buttons in title bar
- Folder and file browsing with proper type detection
- Open files with default applications

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

### Calculator (`blackline-calculator`)
Scientific calculator with:
- Basic arithmetic operations (+, -, ×, ÷)
- Advanced functions (sin, cos, tan, log, ln, sqrt, power)
- Memory functions (MC, MR, M+, MS)
- Parentheses support for complex expressions
- Keyboard input support
- Clean, intuitive interface
- Dark theme with green accent

### System Monitor (`blackline-system-monitor`)
Real-time system monitoring with:
- CPU usage graph and percentage display
- Memory usage (total, used, free, cached, available)
- Process list with PID, name, CPU%, and memory%
- Auto-refresh every 2 seconds
- Visual graphs for CPU and memory trends
- Dark theme with green accents
- Draggable window with minimize/maximize/close buttons

### Web Browser - VoidFox (`voidfox`)
Lightweight web browser built with WebKitGTK, featuring a full application menu and built-in management tabs:
- **Tabbed browsing** with multiple tabs and close buttons on each tab
- **Navigation controls** (back, forward, reload, stop)
- **Smart URL bar** – automatically detects if input is a URL or search term
- **Home button** with customizable home page
- **Search button** for quick web searches
- **Application menu** (☰) with:
  - New window / New private window
  - Bookmarks, History, Downloads, Passwords, Extensions and themes
  - Print, Find in page, Zoom controls, Settings, Report broken site
- **Bookmarks manager** – add, manage, and open bookmarks; persistent storage in `bookmarks.txt`
- **History viewer** – browse visited pages with timestamps; clear history
- **Downloads manager** – track download progress; open download folder; persistent storage in `downloads.txt`
- **Password manager** – store and manage site credentials; copy to clipboard; show/hide passwords; persistent storage in `passwords.txt`
- **Extensions placeholder** – future extension support
- Custom title bar with minimize/maximize/close buttons
- Dark theme with green accent (#00ff88)
- Draggable window (click anywhere on title bar to drag)
- Session management (remembers tabs between sessions)

### Tools Container (`blackline-tools`)
Utility window providing quick access to all BlackLine tools:
- **File Manager** – Launches `blackline-fm`
- **Text Editor** – Launches `blackline-editor`
- **Calculator** – Launches `blackline-calculator`
- **System Monitor** – Launches `blackline-system-monitor`
- **Web Browser** – Launches `voidfox`
- Clean, icon-based interface with emoji icons for easy recognition
- Close button and focus-out auto-close
- Dark theme matching the desktop aesthetic
- Draggable window (click anywhere to drag)
- Automatically closes after launching an application

### Wallpaper Service (`blackline-background`)
Continuous background setter that:
- Uses `feh` to set wallpaper from `~/Desktop/LIDE/images/wal1.png`
- Falls back to solid color (#0b0f14) if wallpaper not found
- Runs continuously to ensure wallpaper persists
- Prevents window managers from overriding the background

### Session Manager (`blackline-session`)
Starts all components in the correct order:
- Wallpaper service → Panel → Window Manager
- Proper process management and cleanup on exit
- Handles SIGINT and SIGTERM gracefully

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