// settings.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "voidfox.h"  // This defines BrowserWindow

typedef enum {
    SEARCH_GOOGLE,
    SEARCH_DUCKDUCKGO,
    SEARCH_BING,
    SEARCH_YAHOO,
    SEARCH_CUSTOM
} SearchEngine;

typedef enum {
    STARTUP_NEW_TAB_PAGE,
    STARTUP_CONTINUE,
    STARTUP_HOME_PAGE,
    STARTUP_SPECIFIC_PAGES
} StartupBehavior;

typedef struct {
    // General
    char *home_page;
    StartupBehavior startup_behavior;
    GList *startup_pages;          // list of strings for multiple pages (optional)

    // Search
    SearchEngine search_engine;
    char *custom_search_url;

    // Appearance
    gboolean dark_mode;
    int font_size;                  // in points
    char *font_family;
    int zoom_level;                  // percentage (100 = 100%)
    gboolean show_bookmarks_bar;
    gboolean show_home_button;

    // Privacy & Security
    gboolean enable_javascript;
    gboolean enable_cookies;
    gboolean enable_images;
    gboolean enable_popups;
    gboolean enable_plugins;         // media/plugins
    gboolean do_not_track;
    gboolean safe_browsing_enabled;
    gboolean send_usage_stats;
    gboolean predictive_search;      // preload search results
    gboolean remember_passwords;     // password manager
    gboolean autofill_enabled;       // form autofill

    // Permissions
    gboolean location_enabled;
    gboolean camera_enabled;
    gboolean microphone_enabled;
    gboolean notifications_enabled;

    // Downloads
    char *download_dir;
    gboolean ask_download_location;

    // System
    gboolean hardware_acceleration;
    char *user_agent;
    gboolean use_system_title_bar;
    gboolean open_links_in_background;

    // History
    gboolean remember_history;
} Settings;

extern Settings settings;

void load_settings(void);
void save_settings(void);
void apply_settings_to_web_view(WebKitWebView *web_view);
const char* get_search_url(const char *query);
void show_settings_dialog(BrowserWindow *browser);

#endif