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

# Tools container sources
TOOLS_SOURCES = tools/tools_container.c tools/auto.c tools/window_resize.c
TOOLS_HEADERS = tools/auto.h tools/minimized_container.h tools/window_resize.h

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

# Tools Container - includes auto.c for app detection
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

# Help
help:
	@echo "Blackline Desktop Environment - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build all components"
	@echo "  make clean        - Remove all binaries and object files"
	@echo ""
	@echo "Components built:"
	@echo "  - blackline-wm           (Window Manager)"
	@echo "  - blackline-panel        (System Panel with WiFi, Network stats, Clock)"
	@echo "  - blackline-launcher     (Application Launcher)"
	@echo "  - blackline-tools        (Tools Container with dynamic app detection)"
	@echo "  - blackline-background   (Background Setter)"
	@echo "  - blackline-fm           (File Manager with Trash support)"
	@echo "  - blackline-editor       (Text Editor)"
	@echo "  - blackline-calculator   (Calculator)"
	@echo "  - blackline-system-monitor (System Monitor)"
	@echo "  - blackline-terminal     (Terminal Emulator)"
	@echo "  - blackline-image-viewer (Image Viewer)"
	@echo "  - blackline-settings     (Settings Tool)"
	@echo "  - voidfox                (Web Browser)"
	@echo "  - firefox-wrapper        (Firefox Wrapper)"
	@echo ""
	@echo "Features:"
	@echo "  - Tools container automatically detects installed applications from /usr/share/applications/"
	@echo "  - File manager with trash support (deleted files go to ~/.local/share/Trash)"
	@echo "  - Right-click context menu with file operations (Cut, Copy, Paste, Delete, Properties)"
	@echo "  - Empty Trash option when right-clicking on Trash directory"
	@echo "  - Window manager with Imlib2 wallpaper support"
	@echo "  - Panel with CPU, RAM, network speeds, WiFi scanning, and date/time"
	@echo "  - Settings tool with display configuration (orientation, resolution, refresh rate)"
	@echo "  - VoidFox web browser with tabs, bookmarks, history, and downloads"
	@echo "  - Image viewer with crop, rotate, flip, and zoom features"
	@echo ""

.PHONY: all clean help