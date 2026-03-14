#ifndef VOIDFOX_EXTENSIONS_H
#define VOIDFOX_EXTENSIONS_H

#include <gtk/gtk.h>
#include "voidfox.h"

// Theme structure
typedef struct {
    const char *name;
    const char *description;
    const char *bg_color;
    const char *text_color;
    const char *entry_bg;
    const char *entry_text;
    const char *entry_border;
    const char *button_bg;
    const char *button_text;
    const char *button_hover;
    const char *title_bar_bg;
    const char *title_bar_border;
    const char *bookmarks_bar_bg;
} BrowserTheme;

// Function prototypes
void show_extensions_tab(BrowserWindow *browser);
void show_themes_tab(BrowserWindow *browser);
void apply_theme(BrowserWindow *browser, const BrowserTheme *theme);

#endif