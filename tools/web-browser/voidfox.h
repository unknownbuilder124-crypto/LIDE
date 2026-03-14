// voidfox.h
#ifndef VOIDFOX_H
#define VOIDFOX_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *url_entry;
    GtkWidget *back_button;
    GtkWidget *forward_button;
    GtkWidget *reload_button;
    GtkWidget *stop_button;
    GtkWidget *app_menu_button;
    GtkWidget *bookmarks_button;
    GtkWidget *home_button;
    GtkWidget *title_bar;
    GtkWidget *bookmarks_bar;

    // For dragging and resizing
    int is_dragging;
    int is_resizing;
    int resize_edge;
    int drag_start_x;
    int drag_start_y;
} BrowserWindow;

typedef struct {
    WebKitWebView *web_view;
    GtkWidget *tab_label;
    GtkWidget *close_button;
} BrowserTab;

// Function declarations
void voidfox_activate(GtkApplication *app, gpointer user_data);
void new_tab(BrowserWindow *browser, const char *url);
void close_tab(BrowserWindow *browser, GtkWidget *tab_child);
void load_url(BrowserWindow *browser, const char *text);
void go_back(BrowserWindow *browser);
void go_forward(BrowserWindow *browser);
void reload_page(BrowserWindow *browser);
void stop_loading(BrowserWindow *browser);
void update_navigation_buttons(BrowserWindow *browser);
void settings_updated(BrowserWindow *browser);

#endif