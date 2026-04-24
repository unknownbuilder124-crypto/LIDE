CC = gcc
CFLAGS = -Wall -O2 -g -I. -Iinclude -Itools -Ipanel -Icontrols/optionals
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
X11_LIBS = -lX11
MATH_LIBS = -lm
IMLIB2_LIBS = -lImlib2
PULSE_LIBS = -lpulse -lpulse-mainloop-glib

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

# File Roller source and target
FILE_ROLLER_SOURCES = fileRoller/file-roller.c controls/optionals/FileChooser.c
FILE_ROLLER_HEADERS = fileRoller/file-roller.h controls/optionals/FileChooser.h
FILE_ROLLER_TARGET = blackline-file-roller

SETTINGS_SOURCES = tools/settings/settings.c \
                   tools/settings/display/display_settings.c \
                   tools/settings/display/orientation.c \
                   tools/settings/display/refresh_rate.c \
                   tools/settings/display/resolution.c \
                   tools/settings/display/wallpaper_settings.c \
                   tools/settings/sound/sound_tab.c \
                   tools/settings/sound/test_sound_tab.c \
                   tools/settings/sound/output/output_volume.c \
                   tools/settings/sound/output/balance.c \
                   tools/settings/sound/output/show_output_device.c \
                   tools/settings/sound/input/input_volume.c \
                   tools/settings/sound/input/show_input_device.c \
                   tools/settings/sound/sounds/volume_levels.c \
                   tools/settings/sound/sounds/alern_sounds.c \
                   tools/settings/power/power_settings.c \
                   tools/settings/power/batary.c \
                   tools/settings/power/mode.c \
                   tools/settings/network/network.c \
                   tools/settings/mouse/mouse.c \
                   tools/settings/mouse/mouse_style.c \
                   panel/internet_settings.c \
                   panel/wifi_list.c \
                   panel/wifi_connect.c \
                   panel/connection_details.c

SETTINGS_HEADERS = tools/settings/display/displaySettings.h \
                   tools/settings/display/orientation.h \
                   tools/settings/display/refresh_rate.h \
                   tools/settings/display/resolution.h \
                   tools/settings/display/wallpaper_settings.h \
                   tools/settings/sound/sound.h \
                   tools/settings/sound/test_sound_tab.h \
                   tools/settings/sound/output/output.h \
                   tools/settings/sound/input/input.h \
                   tools/settings/sound/sounds/sound.h \
                   tools/settings/power/p_settings.h \
                   tools/settings/power/batary.h \
                   tools/settings/power/mode.h \
                   tools/settings/network/network.h \
                   tools/settings/mouse/mouse.h \
                   tools/settings/mouse/mouse_style.h

SETTINGS_TARGET = blackline-settings

# Tools container sources
TOOLS_SOURCES = tools/tools_container.c tools/auto.c tools/window_resize.c
TOOLS_HEADERS = tools/auto.h tools/minimized_container.h tools/window_resize.h

# Base targets
all: blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background \
     blackline-fm blackline-editor blackline-calculator blackline-system-monitor \
     voidfox firefox-wrapper blackline-terminal $(IMAGE_VIEWER_TARGET) $(FILE_ROLLER_TARGET) $(SETTINGS_TARGET)

# Window Manager with Imlib2 support and context menu (needs GTK for dialogs)
blackline-wm: wm/wm.c controls/optionals/O_tab.c controls/optionals/FileChooser.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $^ $(X11_LIBS) $(IMLIB2_LIBS) $(GTK_LIBS)

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

# Background - includes glib for config file support
blackline-background: tools/background.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags glib-2.0) -o $@ $< $(shell pkg-config --libs glib-2.0)

# File Manager - includes window_resize.c, recycle_bin for trash, and file-roller-launcher for file handling
FM_SOURCES = tools/file-manager/fm.c \
             tools/file-manager/browser.c \
             tools/file-manager/recycle_bin.c \
             tools/image-viewer/image-viewer-launcher.c \
             fileRoller/file-roller-launcher.c \
             tools/window_resize.c
FM_HEADERS = tools/file-manager/fm.h \
             tools/file-manager/recycle_bin.h \
             tools/image-viewer/image-viewer.h \
             fileRoller/file-roller.h \
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

# File Roller - Universal file viewer for all file types with custom FileChooser
$(FILE_ROLLER_TARGET): $(FILE_ROLLER_SOURCES) $(FILE_ROLLER_HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $(FILE_ROLLER_SOURCES) $(GTK_LIBS)
	@echo "Built File Roller - Universal file viewer with custom file chooser"

# Settings Tool with Display, Sound, Power, Network, and Mouse tabs
$(SETTINGS_TARGET): $(SETTINGS_SOURCES) $(SETTINGS_HEADERS)
	mkdir -p tools/settings/display
	mkdir -p tools/settings/sound/output
	mkdir -p tools/settings/sound/input
	mkdir -p tools/settings/sound/sounds
	mkdir -p tools/settings/power
	mkdir -p tools/settings/network
	mkdir -p tools/settings/mouse
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -Ipanel -o $@ $(SETTINGS_SOURCES) $(GTK_LIBS) $(PULSE_LIBS) -lm -lasound -lX11
	@echo "Built Settings tool with Display, Sound, Power, Network, and Mouse tabs"

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
	      $(FILE_ROLLER_TARGET) $(SETTINGS_TARGET)
	rm -f *.o tools/*.o panel/*.o tools/system-monitor/*.o tools/web-browser/*.o \
	      tools/terminal/*.o launcher/*.o session/*.o tools/image-viewer/*.o \
	      tools/settings/*.o tools/settings/display/*.o tools/settings/sound/*.o \
	      tools/settings/sound/output/*.o tools/settings/sound/input/*.o \
	      tools/settings/sound/sounds/*.o tools/settings/power/*.o tools/settings/network/*.o \
	      tools/settings/mouse/*.o \
	      tools/file-manager/*.o fileRoller/*.o controls/optionals/*.o
	rm -f ~/.config/blackline/tools_view_mode.conf
	@echo "Clean complete!"

# Clean all (including generated headers)
distclean: clean
	rm -f $(NETWORK_STATS_H)
	rm -f panel/*.gch tools/*.gch tools/settings/*.gch tools/settings/display/*.gch \
	      tools/settings/sound/*.gch tools/settings/sound/output/*.gch \
	      tools/settings/sound/input/*.gch tools/settings/sound/sounds/*.gch \
	      tools/settings/power/*.gch tools/settings/network/*.gch \
	      tools/settings/mouse/*.gch \
	      controls/optionals/*.gch
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
	sudo cp $(FILE_ROLLER_TARGET) /usr/local/bin/
	sudo cp $(SETTINGS_TARGET) /usr/local/bin/
	sudo cp voidfox /usr/local/bin/
	sudo cp $(FIREFOX_WRAPPER) /usr/local/bin/lide-firefox
	-test -f blackline-terminal && sudo cp blackline-terminal /usr/local/bin/
	-test -f blackline-session && sudo cp blackline-session /usr/local/bin/
	mkdir -p ~/.local/share/applications
	cp fileRoller/blackline-file-roller.desktop ~/.local/share/applications/ 2>/dev/null || true
	update-desktop-database ~/.local/share/applications/ 2>/dev/null || true
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
	sudo rm -f /usr/local/bin/$(FILE_ROLLER_TARGET)
	sudo rm -f /usr/local/bin/$(SETTINGS_TARGET)
	sudo rm -f /usr/local/bin/voidfox
	sudo rm -f /usr/local/bin/lide-firefox
	sudo rm -f /usr/local/bin/blackline-terminal
	sudo rm -f /usr/local/bin/blackline-session
	rm -f ~/.local/share/applications/blackline-file-roller.desktop
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

run-file-roller: $(FILE_ROLLER_TARGET)
	./$(FILE_ROLLER_TARGET) $(ARGS)

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

# PulseAudio check
check-pulseaudio:
	@echo "Checking PulseAudio installation..."
	@if pkg-config --exists libpulse; then \
		echo "✓ PulseAudio found: $$(pkg-config --modversion libpulse)"; \
	else \
		echo "✗ PulseAudio not found!"; \
		echo "  Install with: sudo apt install libpulse-dev"; \
	fi

# Check all dependencies
check-all: check-webkit check-firefox check-vte check-imlib2 check-network check-pulseaudio
	@echo ""
	@echo "All dependency checks complete!"

.PHONY: all clean distclean install uninstall run-editor run-wm run-calculator run-system-monitor \
        run-voidfox run-firefox run-terminal run-panel run-launcher run-tools run-fm run-image-viewer \
        run-file-roller run-settings run-session check-webkit check-firefox check-vte check-imlib2 check-network \
        check-pulseaudio check-all