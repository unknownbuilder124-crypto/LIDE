#ifndef VOIDFOX_BOOKMARKS_H
#define VOIDFOX_BOOKMARKS_H

#include <gtk/gtk.h>
#include "voidfox.h"

// Bookmark structure
typedef struct {
    char *title;
    char *url;
} Bookmark;

// Function prototypes
GtkWidget* create_bookmarks_menu(BrowserWindow *browser);
void add_bookmark(BrowserWindow *browser, const char *url, const char *title);
void show_bookmarks_menu(GtkWidget *menu, GtkWidget *button);
void save_bookmarks(void);
void load_bookmarks(void);
void show_bookmarks_tab(BrowserWindow *browser);

#endif