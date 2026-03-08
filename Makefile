CC = gcc
CFLAGS = -Wall -O2 -g -I.
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
X11_LIBS = -lX11

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

all: blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background blackline-fm blackline-editor blackline-calculator blackline-system-monitor voidfox firefox-wrapper

blackline-wm: wm/wm.c
	$(CC) $(CFLAGS) -o $@ $< $(X11_LIBS)

blackline-panel: panel/panel.c tools/minimized_container.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ panel/panel.c tools/minimized_container.c $(GTK_LIBS) $(X11_LIBS)

blackline-launcher: launcher/launcher.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

blackline-tools: tools/tools_container.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

blackline-background: tools/background.c
	$(CC) $(CFLAGS) -o $@ $<

blackline-fm: tools/file-manager/fm.c tools/file-manager/browser.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/file-manager/fm.c tools/file-manager/browser.c $(GTK_LIBS)

# Text editor with edit features
blackline-editor: tools/text_editor/editor.c tools/text_editor/edit.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/text_editor/editor.c tools/text_editor/edit.c $(GTK_LIBS)

# Calculator
blackline-calculator: tools/calculator/calculator.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/calculator/calculator.c $(GTK_LIBS) -lm

# System Monitor
blackline-system-monitor: tools/system-monitor/monitor.o tools/system-monitor/cpu.o tools/system-monitor/memory.o tools/system-monitor/processes.o
	$(CC) -o $@ $^ $(GTK_LIBS)

tools/system-monitor/monitor.o: tools/system-monitor/monitor.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

tools/system-monitor/cpu.o: tools/system-monitor/cpu.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

tools/system-monitor/memory.o: tools/system-monitor/memory.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

tools/system-monitor/processes.o: tools/system-monitor/processes.c tools/system-monitor/monitor.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# VoidFox Web Browser with all features including download stats and settings
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

clean:
	rm -f blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background blackline-fm blackline-editor blackline-calculator blackline-system-monitor voidfox $(FIREFOX_WRAPPER)
	rm -f *.o tools/system-monitor/*.o tools/web-browser/*.o

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

help:
	@echo "Blackline Desktop Environment - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all                    - Build all components"
	@echo "  blackline-wm           - Build window manager"
	@echo "  blackline-panel        - Build panel"
	@echo "  blackline-launcher      - Build application launcher"
	@echo "  blackline-tools         - Build tools container"
	@echo "  blackline-background    - Build background setter"
	@echo "  blackline-fm            - Build file manager"
	@echo "  blackline-editor        - Build text editor"
	@echo "  blackline-calculator    - Build calculator"
	@echo "  blackline-system-monitor - Build system monitor"
	@echo "  voidfox                 - Build VoidFox web browser"
	@echo "  firefox-wrapper         - Build Firefox wrapper"
	@echo "  clean                   - Remove all binaries and object files"
	@echo "  install                 - Install all binaries to /usr/local/bin"
	@echo "  uninstall               - Remove all binaries from /usr/local/bin"
	@echo "  check-webkit            - Check WebKitGTK installation"
	@echo "  check-firefox           - Check Firefox installation"
	@echo ""
	@echo "Run targets:"
	@echo "  run-editor              - Run text editor"
	@echo "  run-wm                  - Run window manager"
	@echo "  run-calculator          - Run calculator"
	@echo "  run-system-monitor      - Run system monitor"
	@echo "  run-voidfox             - Run VoidFox web browser"
	@echo "  run-firefox             - Run Firefox wrapper"

.PHONY: all clean install uninstall run-editor run-wm run-calculator run-system-monitor run-voidfox run-firefox check-webkit check-firefox help