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
- Custom black background with red accent theme
- Draggable window (click anywhere to drag)
- Minimize and close buttons in title bar
- Folder and file browsing with proper type detection
- Open files with default applications

### Tools Container (`blackline-tools`)
Utility window with:
- Quick access to common applications:
  - **File Manager** - Launches `blackline-fm`
  - **Terminal** - not added so far...!
  - **Text Editor** -not added so far...!
  - **Calculator** - not added so far...!
  - **System Monitor** - not added so far...!
- Close button and focus-out auto-close
- Dark theme matching the desktop aesthetic
- Draggable window (click anywhere to drag)

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
BlackLine/
├── wm/ # Window manager
│ ├── wm.c # Main WM event loop
│ ├── keybinds.c # Keyboard shortcut handling
│ ├── layout.c # Window layout and decorations
│ └── wm.h # WM headers
├── panel/ # Top panel
│ ├── panel.c # Panel GUI and logic
│ ├── clock.c # Clock update function
│ └── panel.h # Panel headers
├── launcher/ # Application launcher
│ ├── launcher.c # Launcher GUI and .desktop parsing
│ ├── search.c # Search functionality
│ └── launcher.h # Launcher headers
├── tools/ # Tools and utilities
│ ├── file-manager/ # Custom file manager
│ │ ├── fm.c # File manager GUI
│ │ ├── browser.c # File browsing logic
│ │ └── fm.h # File manager headers
│ ├── tools_container.c # Tools window
│ ├── wallpaper.c # Wallpaper setter
│ └── background.c # Continuous wallpaper service
├── session/ # Session manager
│ └── session.c # Process launcher and manager
├── themes/ # Theme files
│ ├── Xresources # xterm configuration
│ └── lide.css # GTK CSS styling
├── images/ # Wallpaper images
│ └── wal1.png # Default wallpaper
├── include/ # Header files (future use)
├── Makefile # Build configuration
├── run.sh # Development test script
├── blackline.desktop # Desktop entry for display managers
└── README.md # This file


## Building from Source

### Dependencies

```bash
# Debian/Ubuntu/Kali
sudo apt update
sudo apt install libx11-dev libgtk-3-dev pkg-config feh xterm \
                 gedit gnome-calculator gnome-system-monitor

Build Commands:
---------------
# Clone the repository
git clone https://github.com/Algorift169/LIDE.git
cd LIDE

# Build all components
make clean
make all

# This creates:
# - blackline-wm        (window manager)
# - blackline-panel     (top panel)
# - blackline-launcher  (app launcher)
# - blackline-tools     (tools container)
# - blackline-fm        (file manager)
# - blackline-background (background service)
# - blackline-session   (session manager)

===================================================
Installation

# Install to /usr/local/bin (system-wide)
sudo make install

# Or install manually
sudo cp blackline-* /usr/local/bin/
sudo cp blackline.desktop /usr/share/xsessions/

====================================================

Running BlackLine
In Xephyr (for testing)

# Run the test script
./run.sh

# Or manually
Xephyr :1 -screen 1024x768 -ac &
export DISPLAY=:1
./blackline-background &
./blackline-panel &
./blackline-wm

--------------------

Theme

BlackLine features a terminal-inspired dark theme:

    Background: #0b0f14 (dark charcoal)

    Accent: #00ff88 (vibrant green)

    File Manager: Black background (#000000) with red accents (#ff3333)

    Text: White (#ffffff) for readability

    Borders: Subtle accent-colored borders

Troubleshooting
Common Issues

Q: The wallpaper doesn't show up.
A: Ensure feh is installed and the image exists at ~/Desktop/LIDE/images/wal1.png.

Q: Keyboard shortcuts don't work.
A: Make sure the window manager is running and has focus. Check for conflicts with other applications.

Q: The file manager won't launch from Tools.
A: Verify blackline-fm was built successfully and is in the same directory as the other binaries.

Q: Windows aren't draggable.
A: Click anywhere on the window (not just the title bar) to drag - this is by design.
Contributing

Contributions are welcome! Areas for improvement:

    Additional desktop tools

    Workspace support

    System tray

    Configuration file support

    Application menu

This is just the start of BlackLine Desktop Environment. The foundation has been laid with a functional window manager, panel, launcher, file manager, and tools. Many more features and tools are planned for the future, including:

    System settings panel

    Notification area

    Workspace switcher

    Application dock

    And much more...

Stay tuned for updates!


