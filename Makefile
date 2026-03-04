CC = gcc
CFLAGS = -Wall -O2 -g -I.
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
X11_LIBS = -lX11

all: blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background blackline-fm

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

clean:
	rm -f blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background blackline-fm

run:
	./run.sh

.PHONY: all clean run