// settings.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "voidfox.h"

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

typedef enum {
    TAB_BEHAVIOR_LAST,      // Reopen last closed tab before closing browser
    TAB_BEHAVIOR_MAINTAIN,  // Maintain multiple tabs
    TAB_BEHAVIOR_SINGLE     // Always single tab mode
} TabBehavior;

typedef enum {
    HTTPS_ONLY_OFF,
    HTTPS_ONLY_ALL_SITES,
    HTTPS_ONLY_STANDARD_PORTS
} HTTPSOnlyMode;

typedef enum {
    CACHE_UNLIMITED,
    CACHE_100MB,
    CACHE_500MB,
    CACHE_1GB,
    CACHE_DISABLE
} CacheSize;

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
    gboolean show_status_bar;
    int theme_color;                 // 0 = default, 1 = light, 2 = sepia, 3 = dark, etc.

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
    gboolean block_third_party_cookies;
    HTTPSOnlyMode https_only_mode;
    gboolean block_mixed_content;
    gboolean isolate_site_data;
    gboolean block_tracking_scripts;
    gboolean block_cryptomining;
    gboolean block_fingerprinting;
    char *blocked_sites;             // comma-separated list

    // Permissions
    gboolean location_enabled;
    gboolean camera_enabled;
    gboolean microphone_enabled;
    gboolean notifications_enabled;
    gboolean clipboard_enabled;
    gboolean midi_enabled;
    gboolean usb_enabled;
    gboolean gyroscope_enabled;

    // Downloads
    char *download_dir;
    gboolean ask_download_location;
    gboolean show_downloads_when_done;

    // Tabs & Windows
    TabBehavior tab_behavior;
    gboolean restore_session_on_startup;
    gboolean enable_tab_groups;

    // Content Settings
    gboolean enable_sound;
    gboolean enable_autoplay_video;
    gboolean enable_autoplay_audio;
    gboolean enable_pdf_viewer;
    gboolean block_ads;
    gboolean enable_reading_mode;

    // Accessibility
    gboolean text_scaled;
    int text_scale_percentage;       // 100 = normal
    gboolean high_contrast_mode;
    gboolean minimize_ui;
    gboolean show_page_colors;

    // Language & Translation
    char *language;                  // e.g., "en", "en-US"
    gboolean enable_translation;
    char *translation_language;

    // System
    gboolean hardware_acceleration;
    char *user_agent;
    gboolean use_system_title_bar;
    gboolean open_links_in_background;
    gboolean enable_system_proxy;

    // Cache & Storage
    CacheSize cache_size;
    gboolean cache_enabled;
    int cookie_expiration_days;      // 0 = session only
    gboolean offline_content_enabled;

    // History & Data
    gboolean remember_history;
    gboolean remember_form_entries;
    int history_days_to_keep;        // 0 = forever
    gboolean clear_cache_on_exit;
    gboolean clear_cookies_on_exit;
    gboolean clear_history_on_exit;

    // Advanced
    char *startup_command;           // custom startup behavior
    gboolean enable_developer_tools;
    gboolean enable_debugging;
} Settings;

extern Settings settings;

void load_settings(void);
void save_settings(void);
void apply_settings_to_web_view(WebKitWebView *web_view);
const char* get_search_url(const char *query);
void show_settings_dialog(BrowserWindow *browser);
void show_settings_tab(BrowserWindow *browser);

#endif