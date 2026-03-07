#ifndef LIDE_FIREFOX_H
#define LIDE_FIREFOX_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Firefox window structure
typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *url_entry;
    GtkWidget *back_button;
    GtkWidget *forward_button;
    GtkWidget *reload_button;
    GtkWidget *home_button;
    WebKitWebView *web_view;
} FirefoxWindow;

// Function prototypes
void firefox_activate(GtkApplication *app, gpointer user_data);
void load_url(FirefoxWindow *firefox, const char *url);
void go_back(FirefoxWindow *firefox);
void go_forward(FirefoxWindow *firefox);
void reload_page(FirefoxWindow *firefox);
void update_navigation_buttons(FirefoxWindow *firefox);

#endif