CC = gcc
CFLAGS = -Wall -O2 -g -I. -Iinclude -Itools
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
X11_LIBS = -lX11
MATH_LIBS = -lm

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

# Firefox wrapper build
FIREFOX_WRAPPER = tools/firefox/firefox-wrapper

# Base targets
all: blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background \
     blackline-fm blackline-editor blackline-calculator blackline-system-monitor \
     voidfox firefox-wrapper blackline-terminal

# Window Manager
blackline-wm: wm/wm.c
	$(CC) $(CFLAGS) -o $@ $< $(X11_LIBS)

# Panel with system stats - using network_stats.c
blackline-panel: panel/panel.c tools/minimized_container.c panel/network_stats.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ panel/panel.c tools/minimized_container.c $(GTK_LIBS) $(X11_LIBS)

# Launcher
blackline-launcher: launcher/launcher.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

# Tools Container - NO ANIMATIONS (removed animation.c dependency)
blackline-tools: tools/tools_container.c tools/viewMode.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/tools_container.c tools/viewMode.c $(GTK_LIBS) $(X11_LIBS) $(MATH_LIBS)

# Background
blackline-background: tools/background.c
	$(CC) $(CFLAGS) -o $@ $<

# File Manager
blackline-fm: tools/file-manager/fm.c tools/file-manager/browser.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/file-manager/fm.c tools/file-manager/browser.c $(GTK_LIBS)

# Text Editor
blackline-editor: tools/text_editor/editor.c tools/text_editor/edit.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/text_editor/editor.c tools/text_editor/edit.c $(GTK_LIBS)

# Calculator
blackline-calculator: tools/calculator/calculator.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/calculator/calculator.c $(GTK_LIBS) $(MATH_LIBS)

# System Monitor
blackline-system-monitor: tools/system-monitor/monitor.o tools/system-monitor/cpu.o \
                          tools/system-monitor/memory.o tools/system-monitor/processes.o
	$(CC) -o $@ $^ $(GTK_LIBS)

tools/system-monitor/monitor.o: tools/system-monitor/monitor.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

tools/system-monitor/cpu.o: tools/system-monitor/cpu.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

tools/system-monitor/memory.o: tools/system-monitor/memory.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

tools/system-monitor/processes.o: tools/system-monitor/processes.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# View Mode
tools/viewMode.o: tools/viewMode.c tools/viewMode.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# Panel network stats module
panel/network_stats.o: panel/network_stats.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# Terminal - only build if VTE is available
ifeq ($(HAVE_VTE),yes)
blackline-terminal: tools/terminal/terminal.o
	$(CC) -o $@ $^ $(GTK_LIBS) $(VTE_LIBS)

tools/terminal/terminal.o: tools/terminal/terminal.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(VTE_CFLAGS) -c $< -o $@
else
blackline-terminal:
	@echo "Skipping terminal build (vte-2.91 not found)"
endif

# VoidFox Web Browser
voidfox: tools/web-browser/voidfox.o tools/web-browser/browser.o tools/web-browser/tab.o \
         tools/web-browser/app_menu.o tools/web-browser/bookmarks.o \
         tools/web-browser/history.o tools/web-browser/downloads.o \
         tools/web-browser/passwords.o tools/web-browser/extensions.o \
         tools/web-browser/download-stats.o tools/web-browser/settings.o
	$(CC) -o $@ $^ $(GTK_LIBS) $(WEBKIT_LIBS)
	@echo "Built VoidFox with $(WEBKIT_PKG)"

tools/web-browser/voidfox.o: tools/web-browser/voidfox.c tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/browser.o: tools/web-browser/browser.c tools/web-browser/voidfox.h \
                            tools/web-browser/app_menu.h tools/web-browser/bookmarks.h \
                            tools/web-browser/history.h tools/web-browser/downloads.h \
                            tools/web-browser/passwords.h tools/web-browser/extensions.h \
                            tools/web-browser/download-stats.h tools/web-browser/settings.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/tab.o: tools/web-browser/tab.c tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/app_menu.o: tools/web-browser/app_menu.c tools/web-browser/app_menu.h \
                             tools/web-browser/voidfox.h tools/web-browser/bookmarks.h \
                             tools/web-browser/history.h tools/web-browser/downloads.h \
                             tools/web-browser/passwords.h tools/web-browser/extensions.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/bookmarks.o: tools/web-browser/bookmarks.c tools/web-browser/bookmarks.h \
                              tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/history.o: tools/web-browser/history.c tools/web-browser/history.h \
                            tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/downloads.o: tools/web-browser/downloads.c tools/web-browser/downloads.h \
                              tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/passwords.o: tools/web-browser/passwords.c tools/web-browser/passwords.h \
                              tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/extensions.o: tools/web-browser/extensions.c tools/web-browser/extensions.h \
                               tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/download-stats.o: tools/web-browser/download-stats.c \
                                   tools/web-browser/download-stats.h \
                                   tools/web-browser/downloads.h \
                                   tools/web-browser/voidfox.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

tools/web-browser/settings.o: tools/web-browser/settings.c tools/web-browser/settings.h \
                             tools/web-browser/voidfox.h tools/web-browser/history.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(WEBKIT_CFLAGS) -c $< -o $@

# Firefox wrapper
firefox-wrapper: $(FIREFOX_WRAPPER).c
	mkdir -p tools/firefox
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $(FIREFOX_WRAPPER) $(FIREFOX_WRAPPER).c $(GTK_LIBS)
	@echo "Built Firefox wrapper"

# Individual object files
%.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background \
	      blackline-fm blackline-editor blackline-calculator blackline-system-monitor \
	      voidfox $(FIREFOX_WRAPPER) blackline-terminal
	rm -f *.o tools/*.o panel/*.o tools/system-monitor/*.o tools/web-browser/*.o tools/terminal/*.o
	rm -f ~/.config/blackline/tools_view_mode.conf

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
	sudo cp voidfox /usr/local/bin/
	sudo cp $(FIREFOX_WRAPPER) /usr/local/bin/lide-firefox
	-test -f blackline-terminal && sudo cp blackline-terminal /usr/local/bin/

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
	sudo rm -f /usr/local/bin/voidfox
	sudo rm -f /usr/local/bin/lide-firefox
	sudo rm -f /usr/local/bin/blackline-terminal
	rm -f ~/.config/blackline/tools_view_mode.conf

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

help:
	@echo "Blackline Desktop Environment - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all                    - Build all components"
	@echo "  blackline-wm           - Build window manager"
	@echo "  blackline-panel        - Build panel with system stats (CPU, RAM, Network)"
	@echo "  blackline-launcher      - Build application launcher"
	@echo "  blackline-tools         - Build tools container with view mode"
	@echo "  blackline-background    - Build background setter"
	@echo "  blackline-fm            - Build file manager"
	@echo "  blackline-editor        - Build text editor"
	@echo "  blackline-terminal      - Build terminal emulator"
	@echo "  blackline-calculator    - Build calculator"
	@echo "  blackline-system-monitor - Build system monitor"
	@echo "  voidfox                 - Build VoidFox web browser"
	@echo "  firefox-wrapper         - Build Firefox wrapper"
	@echo "  clean                   - Remove all binaries and object files"
	@echo "  install                 - Install all binaries to /usr/local/bin"
	@echo "  uninstall               - Remove all binaries from /usr/local/bin"
	@echo "  check-webkit            - Check WebKitGTK installation"
	@echo "  check-firefox           - Check Firefox installation"
	@echo "  check-vte               - Check VTE installation"
	@echo ""
	@echo "Run targets:"
	@echo "  run-editor              - Run text editor"
	@echo "  run-wm                  - Run window manager"
	@echo "  run-calculator          - Run calculator"
	@echo "  run-system-monitor      - Run system monitor"
	@echo "  run-voidfox             - Run VoidFox web browser"
	@echo "  run-firefox             - Run Firefox wrapper"
	@echo "  run-terminal            - Run terminal emulator"
	@echo ""
	@echo "Panel Features:"
	@echo "  - CPU usage with smoothing"
	@echo "  - RAM usage percentage"
	@echo "  - Upload/Download speeds (auto KB/s, MB/s, GB/s)"
	@echo "  - Date and time display"
	@echo "  - Minimized apps container"
	@echo ""
	@echo "View Mode:"
	@echo "  The tools container supports List/Grid view toggle"
	@echo "  View preference is saved in ~/.config/blackline/tools_view_mode.conf"

.PHONY: all clean install uninstall run-editor run-wm run-calculator run-system-monitor \
        run-voidfox run-firefox run-terminal check-webkit check-firefox check-vte help