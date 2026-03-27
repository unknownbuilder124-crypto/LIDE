CC = gcc
CFLAGS = -Wall -O2 -g -I. -Iinclude -Itools
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
X11_LIBS = -lX11
MATH_LIBS = -lm
IMLIB2_LIBS = -lImlib2

# VTE detection for terminal
VTE_PKG := $(shell pkg-config --exists vte-2.91 && echo vte-2.91)
ifneq ($(VTE_PKG),)
    VTE_CFLAGS = $(shell pkg-config --cflags vte-2.91)
    VTE_LIBS = $(shell pkg-config --libs vte-2.91)
    HAVE_VTE = yes
else
    $(warning "vte-2.91 not found, terminal will not be built")
    HAVE_VTE = no
endif

# WebKitGTK auto-detection for VoidFox
WEBKIT_PKG := $(shell pkg-config --exists webkit2gtk-4.1 && echo webkit2gtk-4.1 || \
                     (pkg-config --exists webkit2gtk-4.0 && echo webkit2gtk-4.0) || \
                     echo "")
ifeq ($(WEBKIT_PKG),)
    WEBKIT_CFLAGS = 
    WEBKIT_LIBS = 
else
    WEBKIT_CFLAGS = $(shell pkg-config --cflags $(WEBKIT_PKG))
    WEBKIT_LIBS = $(shell pkg-config --libs $(WEBKIT_PKG))
endif

# NetworkManager detection
NM_PKG := $(shell pkg-config --exists libnm && echo libnm)
ifneq ($(NM_PKG),)
    NM_CFLAGS = $(shell pkg-config --cflags libnm)
    NM_LIBS = $(shell pkg-config --libs libnm)
    HAVE_NM = yes
else
    $(warning "libnm not found, network manager will use nmcli commands")
    NM_CFLAGS = 
    NM_LIBS = 
    HAVE_NM = no
endif

# Firefox wrapper build
FIREFOX_WRAPPER = tools/firefox/firefox-wrapper

# Network stats header file
NETWORK_STATS_H = panel/network_stats.h

# Image Viewer source and target
IMAGE_VIEWER_SOURCES = tools/image-viewer/image-viewer.c
IMAGE_VIEWER_TARGET = blackline-image-viewer

# Settings tool sources
SETTINGS_SOURCES = tools/settings/settings.c \
                   tools/settings/display/display_settings.c \
                   tools/settings/display/orientation.c \
                   tools/settings/display/refresh_rate.c \
                   tools/settings/display/resolution.c

SETTINGS_HEADERS = tools/settings/display/displaySettings.h \
                   tools/settings/display/orientation.h \
                   tools/settings/display/refresh_rate.h \
                   tools/settings/display/resolution.h

SETTINGS_TARGET = blackline-settings

# Base targets
all: blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background \
     blackline-fm blackline-editor blackline-calculator blackline-system-monitor \
     voidfox firefox-wrapper blackline-terminal $(IMAGE_VIEWER_TARGET) $(SETTINGS_TARGET)

# Window Manager with Imlib2 support
blackline-wm: wm/wm.c
	$(CC) $(CFLAGS) -o $@ $< $(X11_LIBS) $(IMLIB2_LIBS)

# Panel with all features - WiFi, network stats, clock, connection details, etc.
PANEL_SOURCES = panel/panel.c \
                panel/network_stats.c \
                tools/minimized_container.c \
                panel/connection_details.c \
                panel/internet_settings.c \
                panel/wifi_list.c \
                panel/wifi_connect.c \
                panel/clock.c \
                panel/network_manager.c \
                panel/upload.c \
                panel/download.c

PANEL_HEADERS = panel/panel.h \
                panel/network_stats.h \
                panel/network_manager.h \
                panel/wifi_list.h \
                panel/wifi_connect.h \
                panel/internet_settings.h \
                panel/connection_details.h \
                panel/upload.h \
                panel/download.h \
                tools/minimized_container.h

blackline-panel: $(PANEL_SOURCES) $(PANEL_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(NM_CFLAGS) -o $@ $(PANEL_SOURCES) $(GTK_LIBS) $(X11_LIBS) $(NM_LIBS)

# Launcher
LAUNCHER_SOURCES = launcher/launcher.c launcher/search.c
LAUNCHER_HEADERS = launcher/launcher.h

blackline-launcher: $(LAUNCHER_SOURCES) $(LAUNCHER_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(LAUNCHER_SOURCES) $(GTK_LIBS)

# Tools Container - includes window_resize.c for drag/resize functionality
TOOLS_SOURCES = tools/tools_container.c tools/viewMode.c tools/window_resize.c
TOOLS_HEADERS = tools/viewMode.h tools/minimized_container.h tools/window_resize.h

blackline-tools: $(TOOLS_SOURCES) $(TOOLS_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(TOOLS_SOURCES) $(GTK_LIBS) $(X11_LIBS) $(MATH_LIBS)

# Background
blackline-background: tools/background.c
	$(CC) $(CFLAGS) -o $@ $<

# File Manager - includes window_resize.c and recycle_bin for trash functionality
FM_SOURCES = tools/file-manager/fm.c \
             tools/file-manager/browser.c \
             tools/file-manager/recycle_bin.c \
             tools/image-viewer/image-viewer-launcher.c \
             tools/window_resize.c
FM_HEADERS = tools/file-manager/fm.h \
             tools/file-manager/recycle_bin.h \
             tools/image-viewer/image-viewer.h \
             tools/window_resize.h

blackline-fm: $(FM_SOURCES) $(FM_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(FM_SOURCES) $(GTK_LIBS)

# Text Editor - includes window_resize.c for drag/resize functionality
EDITOR_SOURCES = tools/text_editor/editor.c tools/text_editor/edit.c tools/window_resize.c
EDITOR_HEADERS = tools/text_editor/edit.h tools/text_editor/text_editor.h tools/window_resize.h

blackline-editor: $(EDITOR_SOURCES) $(EDITOR_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(EDITOR_SOURCES) $(GTK_LIBS)

# Calculator - includes window_resize.c for drag/resize functionality
CALCULATOR_SOURCES = tools/calculator/calculator.c tools/window_resize.c
CALCULATOR_HEADERS = tools/calculator/calculator.h tools/window_resize.h

blackline-calculator: $(CALCULATOR_SOURCES) $(CALCULATOR_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(CALCULATOR_SOURCES) $(GTK_LIBS) $(MATH_LIBS)

# System Monitor - includes window_resize.c for drag/resize functionality
SYSMON_SOURCES = tools/system-monitor/monitor.c \
                 tools/system-monitor/cpu.c \
                 tools/system-monitor/memory.c \
                 tools/system-monitor/processes.c \
                 tools/window_resize.c
SYSMON_HEADERS = tools/system-monitor/monitor.h tools/window_resize.h

blackline-system-monitor: $(SYSMON_SOURCES) $(SYSMON_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(SYSMON_SOURCES) $(GTK_LIBS)

# Image Viewer
$(IMAGE_VIEWER_TARGET): $(IMAGE_VIEWER_SOURCES)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(IMAGE_VIEWER_SOURCES) $(GTK_LIBS)

# Settings Tool
$(SETTINGS_TARGET): $(SETTINGS_SOURCES) $(SETTINGS_HEADERS)
	mkdir -p tools/settings/display
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(SETTINGS_SOURCES) $(GTK_LIBS)
	@echo "Built Settings tool with Display tab"

# Terminal
ifeq ($(HAVE_VTE),yes)
TERMINAL_SOURCES = tools/terminal/terminal.c tools/window_resize.c

blackline-terminal: $(TERMINAL_SOURCES)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(VTE_CFLAGS) -o $@ $(TERMINAL_SOURCES) $(GTK_LIBS) $(VTE_LIBS)
else
blackline-terminal:
	@echo "Skipping terminal build (vte-2.91 not found)"
endif

# VoidFox Web Browser - includes window_resize.c for drag/resize functionality
BROWSER_SOURCES = tools/web-browser/voidfox.c \
                  tools/web-browser/browser.c \
                  tools/web-browser/tab.c \
                  tools/web-browser/app_menu.c \
                  tools/web-browser/bookmarks.c \
                  tools/web-browser/history.c \
                  tools/web-browser/downloads.c \
                  tools/web-browser/passwords.c \
                  tools/web-browser/extensions.c \
                  tools/web-browser/download-stats.c \
                  tools/web-browser/settings.c \
                  tools/window_resize.c

BROWSER_HEADERS = tools/web-browser/voidfox.h \
                  tools/web-browser/app_menu.h \
                  tools/web-browser/bookmarks.h \
                  tools/web-browser/history.h \
                  tools/web-browser/downloads.h \
                  tools/web-browser/passwords.h \
                  tools/web-browser/extensions.h \
                  tools/web-browser/download-stats.h \
                  tools/web-browser/settings.h \
                  tools/web-browser/tab.h \
                  tools/window_resize.h

voidfox: $(BROWSER_SOURCES) $(BROWSER_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -o $@ $(BROWSER_SOURCES) $(GTK_LIBS) $(WEBKIT_LIBS)
	@echo "Built VoidFox with $(WEBKIT_PKG)"

# Firefox wrapper
FIREFOX_SOURCES = tools/firefox/firefox-wrapper.c
FIREFOX_HEADERS = tools/firefox/firefox.h

firefox-wrapper: $(FIREFOX_SOURCES) $(FIREFOX_HEADERS)
	mkdir -p tools/firefox
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $(FIREFOX_WRAPPER) $(FIREFOX_SOURCES) $(GTK_LIBS)
	@echo "Built Firefox wrapper"

# Session manager (optional)
SESSION_SOURCES = session/session.c

blackline-session: $(SESSION_SOURCES)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(SESSION_SOURCES) $(GTK_LIBS) $(X11_LIBS)

# Create network_stats.h if it doesn't exist
$(NETWORK_STATS_H):
	@echo "Creating network_stats.h..."
	@mkdir -p panel
	@echo '#ifndef NETWORK_STATS_H' > $@
	@echo '#define NETWORK_STATS_H' >> $@
	@echo '' >> $@
	@echo '#include <glib.h>' >> $@
	@echo '' >> $@
	@echo 'void network_stats_init(void);' >> $@
	@echo 'void network_stats_update(void);' >> $@
	@echo 'void network_stats_cleanup(void);' >> $@
	@echo 'double network_stats_get_upload(void);' >> $@
	@echo 'double network_stats_get_download(void);' >> $@
	@echo '' >> $@
	@echo '#endif' >> $@

# Ensure network_stats.h exists before building panel
blackline-panel: $(NETWORK_STATS_H)

# Clean
clean:
	rm -f blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background \
	      blackline-fm blackline-editor blackline-calculator blackline-system-monitor \
	      voidfox $(FIREFOX_WRAPPER) blackline-terminal blackline-session $(IMAGE_VIEWER_TARGET) \
	      $(SETTINGS_TARGET)
	rm -f *.o tools/*.o panel/*.o tools/system-monitor/*.o tools/web-browser/*.o \
	      tools/terminal/*.o launcher/*.o session/*.o tools/image-viewer/*.o \
	      tools/settings/*.o tools/settings/display/*.o tools/file-manager/*.o
	rm -f ~/.config/blackline/tools_view_mode.conf
	@echo "Clean complete!"

# Clean all (including generated headers)
distclean: clean
	rm -f $(NETWORK_STATS_H)
	rm -f panel/*.gch tools/*.gch tools/settings/*.gch tools/settings/display/*.gch
	@echo "Removed generated header files"

# Install all binaries
install: all
	sudo cp blackline-wm /usr/local/bin/
	sudo cp blackline-panel /usr/local/bin/
	sudo cp blackline-launcher /usr/local/bin/
	sudo cp blackline-tools /usr/local/bin/
	sudo cp blackline-background /usr/local/bin/
	sudo cp blackline-fm /usr/local/bin/
	sudo cp blackline-editor /usr/local/bin/
	sudo cp blackline-calculator /usr/local/bin/
	sudo cp blackline-system-monitor /usr/local/bin/
	sudo cp $(IMAGE_VIEWER_TARGET) /usr/local/bin/
	sudo cp $(SETTINGS_TARGET) /usr/local/bin/
	sudo cp voidfox /usr/local/bin/
	sudo cp $(FIREFOX_WRAPPER) /usr/local/bin/lide-firefox
	-test -f blackline-terminal && sudo cp blackline-terminal /usr/local/bin/
	-test -f blackline-session && sudo cp blackline-session /usr/local/bin/
	@echo "Installation complete!"

# Uninstall all binaries
uninstall:
	sudo rm -f /usr/local/bin/blackline-wm
	sudo rm -f /usr/local/bin/blackline-panel
	sudo rm -f /usr/local/bin/blackline-launcher
	sudo rm -f /usr/local/bin/blackline-tools
	sudo rm -f /usr/local/bin/blackline-background
	sudo rm -f /usr/local/bin/blackline-fm
	sudo rm -f /usr/local/bin/blackline-editor
	sudo rm -f /usr/local/bin/blackline-calculator
	sudo rm -f /usr/local/bin/blackline-system-monitor
	sudo rm -f /usr/local/bin/$(IMAGE_VIEWER_TARGET)
	sudo rm -f /usr/local/bin/$(SETTINGS_TARGET)
	sudo rm -f /usr/local/bin/voidfox
	sudo rm -f /usr/local/bin/lide-firefox
	sudo rm -f /usr/local/bin/blackline-terminal
	sudo rm -f /usr/local/bin/blackline-session
	rm -f ~/.config/blackline/tools_view_mode.conf
	@echo "Uninstallation complete!"

# Run commands
run-editor: blackline-editor
	./blackline-editor

run-wm: blackline-wm
	./blackline-wm

run-calculator: blackline-calculator
	./blackline-calculator

run-system-monitor: blackline-system-monitor
	./blackline-system-monitor

run-voidfox: voidfox
	./voidfox

run-firefox: firefox-wrapper
	./$(FIREFOX_WRAPPER)

run-terminal: blackline-terminal
	./blackline-terminal

run-panel: blackline-panel
	./blackline-panel

run-launcher: blackline-launcher
	./blackline-launcher

run-tools: blackline-tools
	./blackline-tools

run-fm: blackline-fm
	./blackline-fm

run-image-viewer: $(IMAGE_VIEWER_TARGET)
	./$(IMAGE_VIEWER_TARGET) $(ARGS)

run-settings: $(SETTINGS_TARGET)
	./$(SETTINGS_TARGET)

run-session: blackline-session
	./blackline-session

# WebKit dependency check
check-webkit:
	@echo "Checking WebKitGTK installation..."
	@if pkg-config --exists webkit2gtk-4.1; then \
		echo "✓ WebKitGTK 4.1 found: $$(pkg-config --modversion webkit2gtk-4.1)"; \
	elif pkg-config --exists webkit2gtk-4.0; then \
		echo "✓ WebKitGTK 4.0 found: $$(pkg-config --modversion webkit2gtk-4.0)"; \
	else \
		echo "✗ WebKitGTK not found!"; \
		echo "  On Debian/Ubuntu: sudo apt install libwebkit2gtk-4.0-dev"; \
		echo "  On Kali Linux: sudo apt install libwebkit2gtk-4.1-dev"; \
		echo "  On Fedora: sudo dnf install webkit2gtk3-devel"; \
		echo "  On Arch: sudo pacman -S webkit2gtk"; \
	fi

# Firefox check
check-firefox:
	@echo "Checking Firefox installation..."
	@if command -v firefox >/dev/null 2>&1; then \
		echo "✓ Firefox found: $$(firefox --version | head -1)"; \
	elif command -v firefox-esr >/dev/null 2>&1; then \
		echo "✓ Firefox ESR found: $$(firefox-esr --version | head -1)"; \
	else \
		echo "✗ Firefox not found!"; \
		echo "  Install with: sudo apt install firefox"; \
		echo "  or: sudo apt install firefox-esr"; \
	fi

# VTE check
check-vte:
	@echo "Checking VTE installation..."
	@if pkg-config --exists vte-2.91; then \
		echo "✓ VTE 2.91 found: $$(pkg-config --modversion vte-2.91)"; \
	else \
		echo "✗ VTE not found!"; \
		echo "  Install with: sudo apt install libvte-2.91-dev"; \
	fi

# Imlib2 check
check-imlib2:
	@echo "Checking Imlib2 installation..."
	@if pkg-config --exists imlib2; then \
		echo "✓ Imlib2 found: $$(pkg-config --modversion imlib2)"; \
	else \
		echo "✗ Imlib2 not found!"; \
		echo "  Install with: sudo apt install libimlib2-dev"; \
		echo "  On Fedora: sudo dnf install imlib2-devel"; \
		echo "  On Arch: sudo pacman -S imlib2"; \
	fi

# NetworkManager check
check-network:
	@echo "Checking NetworkManager installation..."
	@if pkg-config --exists libnm; then \
		echo "✓ NetworkManager found: $$(pkg-config --modversion libnm)"; \
	else \
		echo "✗ NetworkManager not found!"; \
		echo "  Install with: sudo apt install libnm-dev"; \
	fi

# Check all dependencies
check-all: check-webkit check-firefox check-vte check-imlib2 check-network
	@echo ""
	@echo "All dependency checks complete!"

# Help
help:
	@echo "Blackline Desktop Environment - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all                    - Build all components"
	@echo "  blackline-wm           - Build window manager (with Imlib2 wallpaper support)"
	@echo "  blackline-panel        - Build panel with system stats, WiFi, network monitor"
	@echo "  blackline-launcher     - Build application launcher"
	@echo "  blackline-tools        - Build tools container with view mode"
	@echo "  blackline-background   - Build background setter"
	@echo "  blackline-fm           - Build file manager (with trash support)"
	@echo "  blackline-editor       - Build text editor"
	@echo "  blackline-terminal     - Build terminal emulator"
	@echo "  blackline-calculator   - Build calculator"
	@echo "  blackline-system-monitor - Build system monitor (CPU, Memory, Processes)"
	@echo "  blackline-image-viewer - Build image viewer with crop, rotate, flip features"
	@echo "  blackline-settings     - Build settings tool with display configuration"
	@echo "  blackline-session      - Build session manager"
	@echo "  voidfox                - Build VoidFox web browser"
	@echo "  firefox-wrapper        - Build Firefox wrapper"
	@echo "  clean                  - Remove all binaries and object files"
	@echo "  distclean              - Remove all binaries, objects, and generated headers"
	@echo "  install                - Install all binaries to /usr/local/bin"
	@echo "  uninstall              - Remove all binaries from /usr/local/bin"
	@echo "  check-webkit           - Check WebKitGTK installation"
	@echo "  check-firefox          - Check Firefox installation"
	@echo "  check-vte              - Check VTE installation"
	@echo "  check-imlib2           - Check Imlib2 installation"
	@echo "  check-network          - Check NetworkManager installation"
	@echo "  check-all              - Check all dependencies"
	@echo ""
	@echo "Run targets:"
	@echo "  run-editor             - Run text editor"
	@echo "  run-wm                 - Run window manager"
	@echo "  run-calculator         - Run calculator"
	@echo "  run-system-monitor     - Run system monitor"
	@echo "  run-voidfox            - Run VoidFox web browser"
	@echo "  run-firefox            - Run Firefox wrapper"
	@echo "  run-terminal           - Run terminal emulator"
	@echo "  run-panel              - Run panel (for testing)"
	@echo "  run-launcher           - Run launcher (for testing)"
	@echo "  run-tools              - Run tools container (for testing)"
	@echo "  run-fm                 - Run file manager (for testing)"
	@echo "  run-image-viewer       - Run image viewer (use ARGS=filename.jpg to open a file)"
	@echo "  run-settings           - Run settings tool"
	@echo "  run-session            - Run session manager (for testing)"
	@echo ""
	@echo "File Manager Features (new):"
	@echo "  - Right-click context menu with file operations"
	@echo "  - Trash support: deleted files go to ~/.local/share/Trash"
	@echo "  - Empty Trash option when right-clicking on Trash directory"
	@echo "  - Cut, Copy, Paste operations"
	@echo "  - New Folder/File creation"
	@echo "  - Properties dialog with file info"
	@echo "  - Open in Terminal"
	@echo "  - Navigation history (back/forward)"
	@echo "  - Sidebar with Home, Root, Recent, Starred, Trash"
	@echo "  - Drag and drop window movement/resizing"
	@echo ""
	@echo "Settings Tool Features:"
	@echo "  - Display tab with:"
	@echo "    * Orientation (Landscape, Portrait, etc.)"
	@echo "    * Refresh rate selection (60Hz, 75Hz, 120Hz, 144Hz, 240Hz)"
	@echo "    * Resolution selection (1920x1080, 1366x768, etc.)"
	@echo "  - Other tabs (Mouse, Network, Sound, Power, Privacy, Search, Wi-Fi, Bluetooth)"
	@echo "  - Modular design for easy extension"
	@echo ""
	@echo "Panel Features:"
	@echo "  - CPU usage with smoothing"
	@echo "  - RAM usage percentage"
	@echo "  - Upload/Download speeds (auto KB/s, MB/s, GB/s)"
	@echo "  - Date and time display"
	@echo "  - WiFi network scanning and connection"
	@echo "  - Connection details with IP, MAC, interface info"
	@echo "  - Internet settings and configuration"
	@echo "  - Network stats monitoring"
	@echo "  - Minimized apps container"
	@echo ""
	@echo "Window Manager Features:"
	@echo "  - Imlib2 wallpaper support (PNG, JPEG, GIF)"
	@echo "  - Desktop icons from ~/Desktop"
	@echo "  - Right-click context menu with icons"
	@echo "  - Window dragging and resizing"
	@echo "  - Maximize/unmaximize support"
	@echo "  - Movable desktop tools (launchers)"
	@echo "  - Keyboard bindings"
	@echo "  - Layout management"
	@echo "  - Debug mode support"
	@echo ""
	@echo "Image Viewer Features:"
	@echo "  - Open any image format supported by GTK"
	@echo "  - Crop (drag to select area)"
	@echo "  - Rotate left/right (90°)"
	@echo "  - Flip horizontal/vertical"
	@echo "  - Zoom in/out"
	@echo "  - Fit to window / Actual size"
	@echo "  - Save / Save As with format selection"
	@echo "  - Revert to original"
	@echo "  - Modern header bar with toolbar"
	@echo ""
	@echo "VoidFox Web Browser Features:"
	@echo "  - Tabbed browsing"
	@echo "  - Bookmarks manager"
	@echo "  - History tracking"
	@echo "  - Downloads manager with statistics"
	@echo "  - Password manager"
	@echo "  - Extensions support"
	@echo "  - Settings configuration"
	@echo "  - App menu with all options"
	@echo ""
	@echo "View Mode:"
	@echo "  The tools container supports List/Grid view toggle"
	@echo "  View preference is saved in ~/.config/blackline/tools_view_mode.conf"

.PHONY: all clean distclean install uninstall run-editor run-wm run-calculator run-system-monitor \
        run-voidfox run-firefox run-terminal run-panel run-launcher run-tools run-fm run-image-viewer \
        run-settings run-session check-webkit check-firefox check-vte check-imlib2 check-network check-all help