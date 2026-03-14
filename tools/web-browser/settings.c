#include "settings.h"
#include "history.h"
#include "voidfox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <webkit2/webkit2.h>

#define SETTINGS_FILE "voidfox_settings.txt"
#define CONFIG_DIR ".config/lide/voidfox"

Settings settings;

// Default values
static const char *default_home_page = "about:blank";
static const char *default_custom_search = "https://www.google.com/search?q=%s";
static const char *default_download_dir = "Downloads";
static const char *default_user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
static const char *default_font_family = "Sans";

// Ensure config directory exists
static void ensure_config_dir(void)
{
    char *path = g_build_filename(g_get_home_dir(), CONFIG_DIR, NULL);
    g_mkdir_with_parents(path, 0700);
    g_free(path);
}

// Get full path to settings file
static char* get_settings_path(void)
{
    return g_build_filename(g_get_home_dir(), CONFIG_DIR, SETTINGS_FILE, NULL);
}

// Initialize default settings
static void init_default_settings(void)
{
    // General
    settings.home_page = g_strdup(default_home_page);
    settings.startup_behavior = STARTUP_HOME_PAGE;
    settings.startup_pages = NULL;   // not used unless STARTUP_SPECIFIC_PAGES

    // Search
    settings.search_engine = SEARCH_GOOGLE;
    settings.custom_search_url = g_strdup(default_custom_search);

    // Appearance
    settings.dark_mode = TRUE;
    settings.font_size = 12;
    settings.font_family = g_strdup(default_font_family);
    settings.zoom_level = 100;
    settings.show_bookmarks_bar = FALSE;
    settings.show_home_button = TRUE;
    settings.show_status_bar = TRUE;
    settings.theme_color = 0;

    // Privacy & Security
    settings.enable_javascript = TRUE;
    settings.enable_cookies = TRUE;
    settings.enable_images = TRUE;
    settings.enable_popups = FALSE;
    settings.enable_plugins = TRUE;
    settings.do_not_track = TRUE;
    settings.safe_browsing_enabled = TRUE;
    settings.send_usage_stats = FALSE;
    settings.predictive_search = TRUE;
    settings.remember_passwords = TRUE;
    settings.autofill_enabled = TRUE;
    settings.block_third_party_cookies = FALSE;
    settings.https_only_mode = HTTPS_ONLY_OFF;
    settings.block_mixed_content = TRUE;
    settings.isolate_site_data = FALSE;
    settings.block_tracking_scripts = TRUE;
    settings.block_cryptomining = TRUE;
    settings.block_fingerprinting = FALSE;
    settings.blocked_sites = g_strdup("");

    // Permissions
    settings.location_enabled = FALSE;
    settings.camera_enabled = FALSE;
    settings.microphone_enabled = FALSE;
    settings.notifications_enabled = TRUE;
    settings.clipboard_enabled = FALSE;
    settings.midi_enabled = FALSE;
    settings.usb_enabled = FALSE;
    settings.gyroscope_enabled = FALSE;

    // Downloads
    settings.download_dir = g_strdup(default_download_dir);
    settings.ask_download_location = TRUE;
    settings.show_downloads_when_done = TRUE;

    // Tabs & Windows
    settings.tab_behavior = TAB_BEHAVIOR_MAINTAIN;
    settings.restore_session_on_startup = FALSE;
    settings.enable_tab_groups = TRUE;

    // Content Settings
    settings.enable_sound = TRUE;
    settings.enable_autoplay_video = FALSE;
    settings.enable_autoplay_audio = FALSE;
    settings.enable_pdf_viewer = TRUE;
    settings.block_ads = FALSE;
    settings.enable_reading_mode = TRUE;

    // Accessibility
    settings.text_scaled = FALSE;
    settings.text_scale_percentage = 100;
    settings.high_contrast_mode = FALSE;
    settings.minimize_ui = FALSE;
    settings.show_page_colors = TRUE;

    // Language & Translation
    settings.language = g_strdup("en");
    settings.enable_translation = FALSE;
    settings.translation_language = g_strdup("");

    // System
    settings.hardware_acceleration = FALSE;
    settings.user_agent = g_strdup(default_user_agent);
    settings.use_system_title_bar = TRUE;
    settings.open_links_in_background = FALSE;
    settings.enable_system_proxy = TRUE;

    // Cache & Storage
    settings.cache_size = CACHE_UNLIMITED;
    settings.cache_enabled = TRUE;
    settings.cookie_expiration_days = 0;  // Session cookies
    settings.offline_content_enabled = FALSE;

    // History & Data
    settings.remember_history = TRUE;
    settings.remember_form_entries = TRUE;
    settings.history_days_to_keep = 0;  // Forever
    settings.clear_cache_on_exit = FALSE;
    settings.clear_cookies_on_exit = FALSE;
    settings.clear_history_on_exit = FALSE;

    // Advanced
    settings.startup_command = g_strdup("");
    settings.enable_developer_tools = FALSE;
    settings.enable_debugging = FALSE;
}

// Free any allocated strings inside settings (called before overwriting)
static void free_settings_strings(void)
{
    g_free(settings.home_page);
    g_free(settings.custom_search_url);
    g_free(settings.download_dir);
    g_free(settings.user_agent);
    g_free(settings.font_family);
    g_free(settings.blocked_sites);
    g_free(settings.language);
    g_free(settings.translation_language);
    g_free(settings.startup_command);
    if (settings.startup_pages) {
        g_list_free_full(settings.startup_pages, g_free);
        settings.startup_pages = NULL;
    }
}

// Load settings from file
void load_settings(void)
{
    ensure_config_dir();
    char *path = get_settings_path();
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Settings: no settings file found, using defaults\n");
        init_default_settings();
        save_settings(); // save defaults
        g_free(path);
        return;
    }

    // Start with defaults, then override with file
    init_default_settings();

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        if (strcmp(key, "home_page") == 0) {
            g_free(settings.home_page);
            settings.home_page = g_strdup(value);
        }
        else if (strcmp(key, "startup_behavior") == 0) {
            int sb = atoi(value);
            if (sb >= STARTUP_NEW_TAB_PAGE && sb <= STARTUP_SPECIFIC_PAGES)
                settings.startup_behavior = sb;
        }
        else if (strcmp(key, "startup_pages") == 0) {
            // Simple: split by '|' into a list
            if (settings.startup_pages)
                g_list_free_full(settings.startup_pages, g_free);
            settings.startup_pages = NULL;
            char **parts = g_strsplit(value, "|", 0);
            for (int i = 0; parts[i] != NULL; i++) {
                if (strlen(parts[i]) > 0)
                    settings.startup_pages = g_list_append(settings.startup_pages, g_strdup(parts[i]));
            }
            g_strfreev(parts);
        }
        else if (strcmp(key, "search_engine") == 0) {
            int se = atoi(value);
            if (se >= SEARCH_GOOGLE && se <= SEARCH_CUSTOM)
                settings.search_engine = se;
        }
        else if (strcmp(key, "custom_search_url") == 0) {
            g_free(settings.custom_search_url);
            settings.custom_search_url = g_strdup(value);
        }
        else if (strcmp(key, "dark_mode") == 0) {
            settings.dark_mode = atoi(value) != 0;
        }
        else if (strcmp(key, "font_size") == 0) {
            settings.font_size = atoi(value);
        }
        else if (strcmp(key, "font_family") == 0) {
            g_free(settings.font_family);
            settings.font_family = g_strdup(value);
        }
        else if (strcmp(key, "zoom_level") == 0) {
            settings.zoom_level = atoi(value);
        }
        else if (strcmp(key, "show_bookmarks_bar") == 0) {
            settings.show_bookmarks_bar = atoi(value) != 0;
        }
        else if (strcmp(key, "show_home_button") == 0) {
            settings.show_home_button = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_javascript") == 0) {
            settings.enable_javascript = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_cookies") == 0) {
            settings.enable_cookies = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_images") == 0) {
            settings.enable_images = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_popups") == 0) {
            settings.enable_popups = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_plugins") == 0) {
            settings.enable_plugins = atoi(value) != 0;
        }
        else if (strcmp(key, "do_not_track") == 0) {
            settings.do_not_track = atoi(value) != 0;
        }
        else if (strcmp(key, "safe_browsing_enabled") == 0) {
            settings.safe_browsing_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "send_usage_stats") == 0) {
            settings.send_usage_stats = atoi(value) != 0;
        }
        else if (strcmp(key, "predictive_search") == 0) {
            settings.predictive_search = atoi(value) != 0;
        }
        else if (strcmp(key, "remember_passwords") == 0) {
            settings.remember_passwords = atoi(value) != 0;
        }
        else if (strcmp(key, "autofill_enabled") == 0) {
            settings.autofill_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "block_third_party_cookies") == 0) {
            settings.block_third_party_cookies = atoi(value) != 0;
        }
        else if (strcmp(key, "https_only_mode") == 0) {
            int mode = atoi(value);
            if (mode >= HTTPS_ONLY_OFF && mode <= HTTPS_ONLY_STANDARD_PORTS)
                settings.https_only_mode = mode;
        }
        else if (strcmp(key, "block_mixed_content") == 0) {
            settings.block_mixed_content = atoi(value) != 0;
        }
        else if (strcmp(key, "isolate_site_data") == 0) {
            settings.isolate_site_data = atoi(value) != 0;
        }
        else if (strcmp(key, "block_tracking_scripts") == 0) {
            settings.block_tracking_scripts = atoi(value) != 0;
        }
        else if (strcmp(key, "block_cryptomining") == 0) {
            settings.block_cryptomining = atoi(value) != 0;
        }
        else if (strcmp(key, "block_fingerprinting") == 0) {
            settings.block_fingerprinting = atoi(value) != 0;
        }
        else if (strcmp(key, "blocked_sites") == 0) {
            g_free(settings.blocked_sites);
            settings.blocked_sites = g_strdup(value);
        }
        else if (strcmp(key, "location_enabled") == 0) {
            settings.location_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "camera_enabled") == 0) {
            settings.camera_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "microphone_enabled") == 0) {
            settings.microphone_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "notifications_enabled") == 0) {
            settings.notifications_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "clipboard_enabled") == 0) {
            settings.clipboard_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "midi_enabled") == 0) {
            settings.midi_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "usb_enabled") == 0) {
            settings.usb_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "gyroscope_enabled") == 0) {
            settings.gyroscope_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "download_dir") == 0) {
            g_free(settings.download_dir);
            settings.download_dir = g_strdup(value);
        }
        else if (strcmp(key, "ask_download_location") == 0) {
            settings.ask_download_location = atoi(value) != 0;
        }
        else if (strcmp(key, "show_downloads_when_done") == 0) {
            settings.show_downloads_when_done = atoi(value) != 0;
        }
        else if (strcmp(key, "tab_behavior") == 0) {
            int tb = atoi(value);
            if (tb >= TAB_BEHAVIOR_LAST && tb <= TAB_BEHAVIOR_SINGLE)
                settings.tab_behavior = tb;
        }
        else if (strcmp(key, "restore_session_on_startup") == 0) {
            settings.restore_session_on_startup = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_tab_groups") == 0) {
            settings.enable_tab_groups = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_sound") == 0) {
            settings.enable_sound = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_autoplay_video") == 0) {
            settings.enable_autoplay_video = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_autoplay_audio") == 0) {
            settings.enable_autoplay_audio = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_pdf_viewer") == 0) {
            settings.enable_pdf_viewer = atoi(value) != 0;
        }
        else if (strcmp(key, "block_ads") == 0) {
            settings.block_ads = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_reading_mode") == 0) {
            settings.enable_reading_mode = atoi(value) != 0;
        }
        else if (strcmp(key, "text_scaled") == 0) {
            settings.text_scaled = atoi(value) != 0;
        }
        else if (strcmp(key, "text_scale_percentage") == 0) {
            settings.text_scale_percentage = atoi(value);
        }
        else if (strcmp(key, "high_contrast_mode") == 0) {
            settings.high_contrast_mode = atoi(value) != 0;
        }
        else if (strcmp(key, "minimize_ui") == 0) {
            settings.minimize_ui = atoi(value) != 0;
        }
        else if (strcmp(key, "show_page_colors") == 0) {
            settings.show_page_colors = atoi(value) != 0;
        }
        else if (strcmp(key, "language") == 0) {
            g_free(settings.language);
            settings.language = g_strdup(value);
        }
        else if (strcmp(key, "enable_translation") == 0) {
            settings.enable_translation = atoi(value) != 0;
        }
        else if (strcmp(key, "translation_language") == 0) {
            g_free(settings.translation_language);
            settings.translation_language = g_strdup(value);
        }
        else if (strcmp(key, "hardware_acceleration") == 0) {
            settings.hardware_acceleration = atoi(value) != 0;
        }
        else if (strcmp(key, "user_agent") == 0) {
            g_free(settings.user_agent);
            settings.user_agent = g_strdup(value);
        }
        else if (strcmp(key, "use_system_title_bar") == 0) {
            settings.use_system_title_bar = atoi(value) != 0;
        }
        else if (strcmp(key, "open_links_in_background") == 0) {
            settings.open_links_in_background = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_system_proxy") == 0) {
            settings.enable_system_proxy = atoi(value) != 0;
        }
        else if (strcmp(key, "cache_size") == 0) {
            int cs = atoi(value);
            if (cs >= CACHE_UNLIMITED && cs <= CACHE_DISABLE)
                settings.cache_size = cs;
        }
        else if (strcmp(key, "cache_enabled") == 0) {
            settings.cache_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "cookie_expiration_days") == 0) {
            settings.cookie_expiration_days = atoi(value);
        }
        else if (strcmp(key, "offline_content_enabled") == 0) {
            settings.offline_content_enabled = atoi(value) != 0;
        }
        else if (strcmp(key, "remember_history") == 0) {
            settings.remember_history = atoi(value) != 0;
        }
        else if (strcmp(key, "remember_form_entries") == 0) {
            settings.remember_form_entries = atoi(value) != 0;
        }
        else if (strcmp(key, "history_days_to_keep") == 0) {
            settings.history_days_to_keep = atoi(value);
        }
        else if (strcmp(key, "clear_cache_on_exit") == 0) {
            settings.clear_cache_on_exit = atoi(value) != 0;
        }
        else if (strcmp(key, "clear_cookies_on_exit") == 0) {
            settings.clear_cookies_on_exit = atoi(value) != 0;
        }
        else if (strcmp(key, "clear_history_on_exit") == 0) {
            settings.clear_history_on_exit = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_developer_tools") == 0) {
            settings.enable_developer_tools = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_debugging") == 0) {
            settings.enable_debugging = atoi(value) != 0;
        }
        else if (strcmp(key, "show_status_bar") == 0) {
            settings.show_status_bar = atoi(value) != 0;
        }
        else if (strcmp(key, "theme_color") == 0) {
            settings.theme_color = atoi(value);
        }
        else if (strcmp(key, "startup_command") == 0) {
            g_free(settings.startup_command);
            settings.startup_command = g_strdup(value);
        }
    }
    fclose(f);
    g_free(path);
    printf("Settings: loaded\n");
}

// Save settings to file
void save_settings(void)
{
    ensure_config_dir();
    char *path = get_settings_path();
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Settings: failed to save\n");
        g_free(path);
        return;
    }

    // General
    fprintf(f, "home_page=%s\n", settings.home_page);
    fprintf(f, "startup_behavior=%d\n", settings.startup_behavior);
    if (settings.startup_pages) {
        GString *pages = g_string_new("");
        for (GList *l = settings.startup_pages; l != NULL; l = l->next) {
            if (pages->len > 0) g_string_append_c(pages, '|');
            g_string_append(pages, (char*)l->data);
        }
        fprintf(f, "startup_pages=%s\n", pages->str);
        g_string_free(pages, TRUE);
    } else {
        fprintf(f, "startup_pages=\n");
    }

    // Search
    fprintf(f, "search_engine=%d\n", settings.search_engine);
    fprintf(f, "custom_search_url=%s\n", settings.custom_search_url);

    // Appearance
    fprintf(f, "dark_mode=%d\n", settings.dark_mode);
    fprintf(f, "font_size=%d\n", settings.font_size);
    fprintf(f, "font_family=%s\n", settings.font_family);
    fprintf(f, "zoom_level=%d\n", settings.zoom_level);
    fprintf(f, "show_bookmarks_bar=%d\n", settings.show_bookmarks_bar);
    fprintf(f, "show_home_button=%d\n", settings.show_home_button);

    // Privacy & Security
    fprintf(f, "enable_javascript=%d\n", settings.enable_javascript);
    fprintf(f, "enable_cookies=%d\n", settings.enable_cookies);
    fprintf(f, "enable_images=%d\n", settings.enable_images);
    fprintf(f, "enable_popups=%d\n", settings.enable_popups);
    fprintf(f, "enable_plugins=%d\n", settings.enable_plugins);
    fprintf(f, "do_not_track=%d\n", settings.do_not_track);
    fprintf(f, "safe_browsing_enabled=%d\n", settings.safe_browsing_enabled);
    fprintf(f, "send_usage_stats=%d\n", settings.send_usage_stats);
    fprintf(f, "predictive_search=%d\n", settings.predictive_search);
    fprintf(f, "remember_passwords=%d\n", settings.remember_passwords);
    fprintf(f, "autofill_enabled=%d\n", settings.autofill_enabled);

    // Privacy & Security (Additional)
    fprintf(f, "block_third_party_cookies=%d\n", settings.block_third_party_cookies);
    fprintf(f, "https_only_mode=%d\n", settings.https_only_mode);
    fprintf(f, "block_mixed_content=%d\n", settings.block_mixed_content);
    fprintf(f, "isolate_site_data=%d\n", settings.isolate_site_data);
    fprintf(f, "block_tracking_scripts=%d\n", settings.block_tracking_scripts);
    fprintf(f, "block_cryptomining=%d\n", settings.block_cryptomining);
    fprintf(f, "block_fingerprinting=%d\n", settings.block_fingerprinting);
    fprintf(f, "blocked_sites=%s\n", settings.blocked_sites);

    // Appearance (Additional)
    fprintf(f, "show_status_bar=%d\n", settings.show_status_bar);
    fprintf(f, "theme_color=%d\n", settings.theme_color);

    // Permissions
    fprintf(f, "location_enabled=%d\n", settings.location_enabled);
    fprintf(f, "camera_enabled=%d\n", settings.camera_enabled);
    fprintf(f, "microphone_enabled=%d\n", settings.microphone_enabled);
    fprintf(f, "notifications_enabled=%d\n", settings.notifications_enabled);
    fprintf(f, "clipboard_enabled=%d\n", settings.clipboard_enabled);
    fprintf(f, "midi_enabled=%d\n", settings.midi_enabled);
    fprintf(f, "usb_enabled=%d\n", settings.usb_enabled);
    fprintf(f, "gyroscope_enabled=%d\n", settings.gyroscope_enabled);

    // Downloads
    fprintf(f, "download_dir=%s\n", settings.download_dir);
    fprintf(f, "ask_download_location=%d\n", settings.ask_download_location);
    fprintf(f, "show_downloads_when_done=%d\n", settings.show_downloads_when_done);

    // Tabs & Windows
    fprintf(f, "tab_behavior=%d\n", settings.tab_behavior);
    fprintf(f, "restore_session_on_startup=%d\n", settings.restore_session_on_startup);
    fprintf(f, "enable_tab_groups=%d\n", settings.enable_tab_groups);

    // Content Settings
    fprintf(f, "enable_sound=%d\n", settings.enable_sound);
    fprintf(f, "enable_autoplay_video=%d\n", settings.enable_autoplay_video);
    fprintf(f, "enable_autoplay_audio=%d\n", settings.enable_autoplay_audio);
    fprintf(f, "enable_pdf_viewer=%d\n", settings.enable_pdf_viewer);
    fprintf(f, "block_ads=%d\n", settings.block_ads);
    fprintf(f, "enable_reading_mode=%d\n", settings.enable_reading_mode);

    // Accessibility
    fprintf(f, "text_scaled=%d\n", settings.text_scaled);
    fprintf(f, "text_scale_percentage=%d\n", settings.text_scale_percentage);
    fprintf(f, "high_contrast_mode=%d\n", settings.high_contrast_mode);
    fprintf(f, "minimize_ui=%d\n", settings.minimize_ui);
    fprintf(f, "show_page_colors=%d\n", settings.show_page_colors);

    // Language & Translation
    fprintf(f, "language=%s\n", settings.language);
    fprintf(f, "enable_translation=%d\n", settings.enable_translation);
    fprintf(f, "translation_language=%s\n", settings.translation_language);

    // System
    fprintf(f, "hardware_acceleration=%d\n", settings.hardware_acceleration);
    fprintf(f, "user_agent=%s\n", settings.user_agent);
    fprintf(f, "use_system_title_bar=%d\n", settings.use_system_title_bar);
    fprintf(f, "open_links_in_background=%d\n", settings.open_links_in_background);
    fprintf(f, "enable_system_proxy=%d\n", settings.enable_system_proxy);

    // Cache & Storage
    fprintf(f, "cache_size=%d\n", settings.cache_size);
    fprintf(f, "cache_enabled=%d\n", settings.cache_enabled);
    fprintf(f, "cookie_expiration_days=%d\n", settings.cookie_expiration_days);
    fprintf(f, "offline_content_enabled=%d\n", settings.offline_content_enabled);

    // History & Data
    fprintf(f, "remember_history=%d\n", settings.remember_history);
    fprintf(f, "remember_form_entries=%d\n", settings.remember_form_entries);
    fprintf(f, "history_days_to_keep=%d\n", settings.history_days_to_keep);
    fprintf(f, "clear_cache_on_exit=%d\n", settings.clear_cache_on_exit);
    fprintf(f, "clear_cookies_on_exit=%d\n", settings.clear_cookies_on_exit);
    fprintf(f, "clear_history_on_exit=%d\n", settings.clear_history_on_exit);

    // Advanced
    fprintf(f, "startup_command=%s\n", settings.startup_command);
    fprintf(f, "enable_developer_tools=%d\n", settings.enable_developer_tools);
    fprintf(f, "enable_debugging=%d\n", settings.enable_debugging);

    fclose(f);
    g_free(path);
    printf("Settings: saved\n");
}

// Apply settings to a WebKitWebView
void apply_settings_to_web_view(WebKitWebView *web_view)
{
    WebKitSettings *wk_settings = webkit_web_view_get_settings(web_view);
    if (!wk_settings) return;

    webkit_settings_set_enable_javascript(wk_settings, settings.enable_javascript);
    webkit_settings_set_enable_media(wk_settings, settings.enable_plugins);
    webkit_settings_set_enable_webgl(wk_settings, settings.hardware_acceleration);
    webkit_settings_set_enable_webrtc(wk_settings, settings.hardware_acceleration);
    webkit_settings_set_auto_load_images(wk_settings, settings.enable_images);
    webkit_settings_set_enable_page_cache(wk_settings, TRUE);
    webkit_settings_set_enable_smooth_scrolling(wk_settings, TRUE);

    // Font settings
    webkit_settings_set_default_font_size(wk_settings, settings.font_size);
    if (settings.font_family && *settings.font_family) {
        webkit_settings_set_default_font_family(wk_settings, settings.font_family);
    }

    // Zoom level
    webkit_web_view_set_zoom_level(web_view, settings.zoom_level / 100.0);

    // Cookies
    WebKitCookieManager *cookie_manager = webkit_web_context_get_cookie_manager(webkit_web_view_get_context(web_view));
    if (cookie_manager) {
        if (settings.enable_cookies) {
            char *cookies_path = g_build_filename(g_get_home_dir(), CONFIG_DIR, "cookies.txt", NULL);
            webkit_cookie_manager_set_persistent_storage(cookie_manager,
                cookies_path,
                WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
            g_free(cookies_path);
        } else {
            webkit_cookie_manager_set_persistent_storage(cookie_manager, NULL, WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
        }
    }

    // User agent
    if (settings.user_agent && *settings.user_agent) {
        webkit_settings_set_user_agent(wk_settings, settings.user_agent);
    }
}

// Construct search URL based on settings and query
const char* get_search_url(const char *query)
{
    static GString *url = NULL;
    if (url) g_string_free(url, TRUE);
    url = g_string_new("");

    char *encoded = g_uri_escape_string(query, NULL, FALSE);
    switch (settings.search_engine) {
        case SEARCH_GOOGLE:
            g_string_printf(url, "https://www.google.com/search?q=%s", encoded);
            break;
        case SEARCH_DUCKDUCKGO:
            g_string_printf(url, "https://duckduckgo.com/?q=%s", encoded);
            break;
        case SEARCH_BING:
            g_string_printf(url, "https://www.bing.com/search?q=%s", encoded);
            break;
        case SEARCH_YAHOO:
            g_string_printf(url, "https://search.yahoo.com/search?p=%s", encoded);
            break;
        case SEARCH_CUSTOM:
            if (settings.custom_search_url && *settings.custom_search_url) {
                g_string_printf(url, settings.custom_search_url, encoded);
            } else {
                g_string_printf(url, "https://www.google.com/search?q=%s", encoded);
            }
            break;
        default:
            g_string_printf(url, "https://www.google.com/search?q=%s", encoded);
            break;
    }
    g_free(encoded);
    return url->str;
}

// ----------------------------------------------------------------------
// Settings Dialog
// ----------------------------------------------------------------------

typedef struct {
    GtkWidget *dialog;

    // General
    GtkWidget *home_entry;
    GtkWidget *startup_newtab_radio;
    GtkWidget *startup_continue_radio;
    GtkWidget *startup_home_radio;
    GtkWidget *startup_specific_radio;
    GtkWidget *startup_specific_entry;

    // Search
    GtkWidget *search_combo;
    GtkWidget *custom_search_entry;
    GtkWidget *predictive_check;

    // Appearance
    GtkWidget *dark_mode_check;
    GtkWidget *font_size_spin;
    GtkWidget *font_family_entry;
    GtkWidget *zoom_spin;
    GtkWidget *bookmarks_bar_check;
    GtkWidget *home_button_check;
    GtkWidget *status_bar_check;
    GtkWidget *theme_color_combo;

    // Privacy & Security
    GtkWidget *js_check;
    GtkWidget *cookies_check;
    GtkWidget *images_check;
    GtkWidget *popups_check;        // "Block pop-ups" (inverted logic)
    GtkWidget *plugins_check;
    GtkWidget *dnt_check;
    GtkWidget *safe_browsing_check;
    GtkWidget *usage_stats_check;
    GtkWidget *passwords_check;
    GtkWidget *autofill_check;
    GtkWidget *block_third_party_cookies_check;
    GtkWidget *https_only_combo;
    GtkWidget *block_mixed_content_check;
    GtkWidget *isolate_site_data_check;
    GtkWidget *block_tracking_scripts_check;
    GtkWidget *block_cryptomining_check;
    GtkWidget *block_fingerprinting_check;
    GtkWidget *blocked_sites_entry;

    // Permissions
    GtkWidget *location_check;
    GtkWidget *camera_check;
    GtkWidget *microphone_check;
    GtkWidget *notifications_check;
    GtkWidget *clipboard_check;
    GtkWidget *midi_check;
    GtkWidget *usb_check;
    GtkWidget *gyroscope_check;

    // Downloads
    GtkWidget *download_dir_entry;
    GtkWidget *download_dir_button;
    GtkWidget *ask_location_check;
    GtkWidget *show_downloads_check;

    // Tabs & Windows
    GtkWidget *tab_behavior_combo;
    GtkWidget *restore_session_check;
    GtkWidget *tab_groups_check;

    // Content
    GtkWidget *enable_sound_check;
    GtkWidget *autoplay_video_check;
    GtkWidget *autoplay_audio_check;
    GtkWidget *pdf_viewer_check;
    GtkWidget *block_ads_check;
    GtkWidget *reading_mode_check;

    // Accessibility
    GtkWidget *text_scaled_check;
    GtkWidget *text_scale_spin;
    GtkWidget *high_contrast_check;
    GtkWidget *minimize_ui_check;
    GtkWidget *show_colors_check;

    // Language
    GtkWidget *language_entry;
    GtkWidget *enable_translation_check;
    GtkWidget *translation_lang_entry;

    // System
    GtkWidget *hw_accel_check;
    GtkWidget *user_agent_entry;
    GtkWidget *system_title_bar_check;
    GtkWidget *open_background_check;
    GtkWidget *system_proxy_check;

    // Cache & Storage
    GtkWidget *cache_size_combo;
    GtkWidget *cache_enabled_check;
    GtkWidget *cookie_expiry_spin;
    GtkWidget *offline_content_check;

    // History
    GtkWidget *history_check;
    GtkWidget *remember_forms_check;
    GtkWidget *history_days_spin;
    GtkWidget *clear_cache_exit_check;
    GtkWidget *clear_cookies_exit_check;
    GtkWidget *clear_history_exit_check;
    GtkWidget *clear_history_btn;

    // Advanced
    GtkWidget *startup_command_entry;
    GtkWidget *dev_tools_check;
    GtkWidget *debugging_check;

    BrowserWindow *browser;
} SettingsDialogData;

// Helper: update sensitivity of custom search entry based on search engine
static void on_search_engine_changed(GtkComboBox *combo, SettingsDialogData *data)
{
    int active = gtk_combo_box_get_active(combo);
    gtk_widget_set_sensitive(data->custom_search_entry, (active == SEARCH_CUSTOM));
}

// Helper: update sensitivity of specific pages entry based on radio
static void on_startup_radio_toggled(GtkToggleButton *button, SettingsDialogData *data)
{
    (void)button;
    gboolean sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->startup_specific_radio));
    gtk_widget_set_sensitive(data->startup_specific_entry, sensitive);
}

// Browse for download folder
static void on_download_dir_clicked(GtkButton *button, SettingsDialogData *data)
{
    (void)button;
    GtkWidget *chooser = gtk_file_chooser_dialog_new("Select Download Directory",
                                                      GTK_WINDOW(data->dialog),
                                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                                      "_Select", GTK_RESPONSE_ACCEPT,
                                                      NULL);
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        gtk_entry_set_text(GTK_ENTRY(data->download_dir_entry), folder);
        g_free(folder);
    }
    gtk_widget_destroy(chooser);
}

// Dialog response handler
static void on_settings_response(GtkDialog *dialog, gint response_id, SettingsDialogData *data)
{
    if (response_id == GTK_RESPONSE_ACCEPT) {
        // ----- General -----
        g_free(settings.home_page);
        settings.home_page = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->home_entry)));

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->startup_newtab_radio)))
            settings.startup_behavior = STARTUP_NEW_TAB_PAGE;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->startup_continue_radio)))
            settings.startup_behavior = STARTUP_CONTINUE;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->startup_home_radio)))
            settings.startup_behavior = STARTUP_HOME_PAGE;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->startup_specific_radio))) {
            settings.startup_behavior = STARTUP_SPECIFIC_PAGES;
            // For simplicity, store a single page as a list
            const char *specific = gtk_entry_get_text(GTK_ENTRY(data->startup_specific_entry));
            if (settings.startup_pages)
                g_list_free_full(settings.startup_pages, g_free);
            settings.startup_pages = g_list_append(NULL, g_strdup(specific));
        }

        // ----- Search -----
        settings.search_engine = gtk_combo_box_get_active(GTK_COMBO_BOX(data->search_combo));
        g_free(settings.custom_search_url);
        settings.custom_search_url = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->custom_search_entry)));
        settings.predictive_search = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->predictive_check));

        // ----- Appearance -----
        settings.dark_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->dark_mode_check));
        settings.font_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->font_size_spin));
        g_free(settings.font_family);
        settings.font_family = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->font_family_entry)));
        settings.zoom_level = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->zoom_spin));
        settings.show_bookmarks_bar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->bookmarks_bar_check));
        settings.show_home_button = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->home_button_check));
        settings.show_status_bar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->status_bar_check));
        settings.theme_color = gtk_combo_box_get_active(GTK_COMBO_BOX(data->theme_color_combo));

        // ----- Privacy & Security -----
        settings.enable_javascript = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->js_check));
        settings.enable_cookies = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->cookies_check));
        settings.enable_images = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->images_check));
        settings.enable_popups = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->popups_check)); // inverted
        settings.enable_plugins = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->plugins_check));
        settings.do_not_track = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->dnt_check));
        settings.safe_browsing_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->safe_browsing_check));
        settings.send_usage_stats = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->usage_stats_check));
        settings.remember_passwords = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->passwords_check));
        settings.autofill_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->autofill_check));
        settings.block_third_party_cookies = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->block_third_party_cookies_check));
        settings.https_only_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(data->https_only_combo));
        settings.block_mixed_content = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->block_mixed_content_check));
        settings.isolate_site_data = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->isolate_site_data_check));
        settings.block_tracking_scripts = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->block_tracking_scripts_check));
        settings.block_cryptomining = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->block_cryptomining_check));
        settings.block_fingerprinting = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->block_fingerprinting_check));
        g_free(settings.blocked_sites);
        settings.blocked_sites = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->blocked_sites_entry)));

        // ----- Permissions -----
        settings.location_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->location_check));
        settings.camera_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->camera_check));
        settings.microphone_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->microphone_check));
        settings.notifications_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->notifications_check));
        settings.clipboard_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->clipboard_check));
        settings.midi_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->midi_check));
        settings.usb_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->usb_check));
        settings.gyroscope_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->gyroscope_check));

        // ----- Downloads -----
        g_free(settings.download_dir);
        settings.download_dir = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->download_dir_entry)));
        settings.ask_download_location = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->ask_location_check));
        settings.show_downloads_when_done = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->show_downloads_check));

        // ----- Tabs & Windows -----
        settings.tab_behavior = gtk_combo_box_get_active(GTK_COMBO_BOX(data->tab_behavior_combo));
        settings.restore_session_on_startup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->restore_session_check));
        settings.enable_tab_groups = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->tab_groups_check));

        // ----- Content Settings -----
        settings.enable_sound = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->enable_sound_check));
        settings.enable_autoplay_video = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->autoplay_video_check));
        settings.enable_autoplay_audio = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->autoplay_audio_check));
        settings.enable_pdf_viewer = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->pdf_viewer_check));
        settings.block_ads = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->block_ads_check));
        settings.enable_reading_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->reading_mode_check));

        // ----- Accessibility -----
        settings.text_scaled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->text_scaled_check));
        settings.text_scale_percentage = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->text_scale_spin));
        settings.high_contrast_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->high_contrast_check));
        settings.minimize_ui = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->minimize_ui_check));
        settings.show_page_colors = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->show_colors_check));

        // ----- Language & Translation -----
        g_free(settings.language);
        settings.language = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->language_entry)));
        settings.enable_translation = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->enable_translation_check));
        g_free(settings.translation_language);
        settings.translation_language = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->translation_lang_entry)));

        // ----- System -----
        settings.hardware_acceleration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->hw_accel_check));
        g_free(settings.user_agent);
        settings.user_agent = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->user_agent_entry)));
        settings.use_system_title_bar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->system_title_bar_check));
        settings.open_links_in_background = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->open_background_check));
        settings.enable_system_proxy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->system_proxy_check));

        // ----- Cache & Storage -----
        settings.cache_size = gtk_combo_box_get_active(GTK_COMBO_BOX(data->cache_size_combo));
        settings.cache_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->cache_enabled_check));
        settings.cookie_expiration_days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->cookie_expiry_spin));
        settings.offline_content_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->offline_content_check));

        // ----- History & Data -----
        settings.remember_history = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->history_check));
        settings.remember_form_entries = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->remember_forms_check));
        settings.history_days_to_keep = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->history_days_spin));
        settings.clear_cache_on_exit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->clear_cache_exit_check));
        settings.clear_cookies_on_exit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->clear_cookies_exit_check));
        settings.clear_history_on_exit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->clear_history_exit_check));

        // ----- Advanced -----
        g_free(settings.startup_command);
        settings.startup_command = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->startup_command_entry)));
        settings.enable_developer_tools = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->dev_tools_check));
        settings.enable_debugging = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->debugging_check));

        save_settings();

        // Apply changes to the current browser window
        if (data->browser) {
            settings_updated(data->browser);
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_free(data);
}

void show_settings_dialog(BrowserWindow *browser)

{
    SettingsDialogData *data = g_new0(SettingsDialogData, 1);
    data->browser = browser;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Settings",
                                                    GTK_WINDOW(browser->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Save", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 750, 600);
    data->dialog = dialog;

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(content), notebook);

    // ------------------------------------------------------------------
    // 1. General tab
    // ------------------------------------------------------------------
    GtkWidget *general = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(general), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general, gtk_label_new("General"));

    // Startup
    GtkWidget *startup_frame = gtk_frame_new("Startup");
    gtk_box_pack_start(GTK_BOX(general), startup_frame, FALSE, FALSE, 0);
    GtkWidget *startup_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(startup_vbox), 5);
    gtk_container_add(GTK_CONTAINER(startup_frame), startup_vbox);

    GSList *group = NULL;
    data->startup_newtab_radio = gtk_radio_button_new_with_label(group, "Open the New Tab page");
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(data->startup_newtab_radio));
    gtk_box_pack_start(GTK_BOX(startup_vbox), data->startup_newtab_radio, FALSE, FALSE, 0);

    data->startup_continue_radio = gtk_radio_button_new_with_label(group, "Continue where you left off");
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(data->startup_continue_radio));
    gtk_box_pack_start(GTK_BOX(startup_vbox), data->startup_continue_radio, FALSE, FALSE, 0);

    data->startup_home_radio = gtk_radio_button_new_with_label(group, "Open the home page");
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(data->startup_home_radio));
    gtk_box_pack_start(GTK_BOX(startup_vbox), data->startup_home_radio, FALSE, FALSE, 0);

    data->startup_specific_radio = gtk_radio_button_new_with_label(group, "Open a specific page or set of pages");
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(data->startup_specific_radio));
    gtk_box_pack_start(GTK_BOX(startup_vbox), data->startup_specific_radio, FALSE, FALSE, 0);

    GtkWidget *specific_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(startup_vbox), specific_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(specific_hbox), gtk_label_new("Enter URL:"), FALSE, FALSE, 0);
    data->startup_specific_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->startup_specific_entry),
                       settings.startup_pages ? (char*)settings.startup_pages->data : "");
    gtk_box_pack_start(GTK_BOX(specific_hbox), data->startup_specific_entry, TRUE, TRUE, 0);

    // Set active radio based on settings.startup_behavior
    switch (settings.startup_behavior) {
        case STARTUP_NEW_TAB_PAGE:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->startup_newtab_radio), TRUE);
            break;
        case STARTUP_CONTINUE:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->startup_continue_radio), TRUE);
            break;
        case STARTUP_HOME_PAGE:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->startup_home_radio), TRUE);
            break;
        case STARTUP_SPECIFIC_PAGES:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->startup_specific_radio), TRUE);
            break;
    }
    g_signal_connect(data->startup_specific_radio, "toggled", G_CALLBACK(on_startup_radio_toggled), data);
    on_startup_radio_toggled(NULL, data); // initial sensitivity

    // Home page
    GtkWidget *home_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), home_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(home_hbox), gtk_label_new("Home page:"), FALSE, FALSE, 0);
    data->home_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->home_entry), settings.home_page);
    gtk_box_pack_start(GTK_BOX(home_hbox), data->home_entry, TRUE, TRUE, 0);

    // ------------------------------------------------------------------
    // 2. Search tab
    // ------------------------------------------------------------------
    GtkWidget *search_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(search_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), search_tab, gtk_label_new("Search"));

    GtkWidget *search_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(search_tab), search_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_hbox), gtk_label_new("Default search engine:"), FALSE, FALSE, 0);
    data->search_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Bing");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Yahoo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Custom");
    gtk_combo_box_set_active(GTK_COMBO_BOX(data->search_combo), settings.search_engine);
    gtk_box_pack_start(GTK_BOX(search_hbox), data->search_combo, FALSE, FALSE, 0);

    GtkWidget *custom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(search_tab), custom_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_hbox), gtk_label_new("Custom URL (use %s for query):"), FALSE, FALSE, 0);
    data->custom_search_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->custom_search_entry), settings.custom_search_url);
    gtk_box_pack_start(GTK_BOX(custom_hbox), data->custom_search_entry, TRUE, TRUE, 0);

    data->predictive_check = gtk_check_button_new_with_label("Predictive search (preload search results)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->predictive_check), settings.predictive_search);
    gtk_box_pack_start(GTK_BOX(search_tab), data->predictive_check, FALSE, FALSE, 0);

    g_signal_connect(data->search_combo, "changed", G_CALLBACK(on_search_engine_changed), data);
    on_search_engine_changed(GTK_COMBO_BOX(data->search_combo), data);

    // ------------------------------------------------------------------
    // 3. Appearance tab
    // ------------------------------------------------------------------
    GtkWidget *appearance = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(appearance), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), appearance, gtk_label_new("Appearance"));

    data->dark_mode_check = gtk_check_button_new_with_label("Dark mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->dark_mode_check), settings.dark_mode);
    gtk_box_pack_start(GTK_BOX(appearance), data->dark_mode_check, FALSE, FALSE, 0);

    GtkWidget *font_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), font_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(font_hbox), gtk_label_new("Font size:"), FALSE, FALSE, 0);
    data->font_size_spin = gtk_spin_button_new_with_range(6, 72, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->font_size_spin), settings.font_size);
    gtk_box_pack_start(GTK_BOX(font_hbox), data->font_size_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(font_hbox), gtk_label_new("Font family:"), FALSE, FALSE, 0);
    data->font_family_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->font_family_entry), settings.font_family);
    gtk_box_pack_start(GTK_BOX(font_hbox), data->font_family_entry, TRUE, TRUE, 0);

    GtkWidget *zoom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), zoom_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_hbox), gtk_label_new("Page zoom (%):"), FALSE, FALSE, 0);
    data->zoom_spin = gtk_spin_button_new_with_range(30, 300, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->zoom_spin), settings.zoom_level);
    gtk_box_pack_start(GTK_BOX(zoom_hbox), data->zoom_spin, FALSE, FALSE, 0);

    data->bookmarks_bar_check = gtk_check_button_new_with_label("Show bookmarks bar");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->bookmarks_bar_check), settings.show_bookmarks_bar);
    gtk_box_pack_start(GTK_BOX(appearance), data->bookmarks_bar_check, FALSE, FALSE, 0);

    data->home_button_check = gtk_check_button_new_with_label("Show home button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->home_button_check), settings.show_home_button);
    gtk_box_pack_start(GTK_BOX(appearance), data->home_button_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // 4. Privacy & Security tab
    // ------------------------------------------------------------------
    GtkWidget *privacy = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(privacy), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), privacy, gtk_label_new("Privacy & Security"));

    data->js_check = gtk_check_button_new_with_label("Enable JavaScript");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->js_check), settings.enable_javascript);
    gtk_box_pack_start(GTK_BOX(privacy), data->js_check, FALSE, FALSE, 0);

    data->cookies_check = gtk_check_button_new_with_label("Enable Cookies");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->cookies_check), settings.enable_cookies);
    gtk_box_pack_start(GTK_BOX(privacy), data->cookies_check, FALSE, FALSE, 0);

    data->images_check = gtk_check_button_new_with_label("Load Images");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->images_check), settings.enable_images);
    gtk_box_pack_start(GTK_BOX(privacy), data->images_check, FALSE, FALSE, 0);

    data->popups_check = gtk_check_button_new_with_label("Block Pop-up Windows");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->popups_check), !settings.enable_popups);
    gtk_box_pack_start(GTK_BOX(privacy), data->popups_check, FALSE, FALSE, 0);

    data->plugins_check = gtk_check_button_new_with_label("Enable Plugins/Media");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->plugins_check), settings.enable_plugins);
    gtk_box_pack_start(GTK_BOX(privacy), data->plugins_check, FALSE, FALSE, 0);

    data->dnt_check = gtk_check_button_new_with_label("Send 'Do Not Track' request");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->dnt_check), settings.do_not_track);
    gtk_box_pack_start(GTK_BOX(privacy), data->dnt_check, FALSE, FALSE, 0);

    data->safe_browsing_check = gtk_check_button_new_with_label("Safe Browsing (block dangerous sites)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->safe_browsing_check), settings.safe_browsing_enabled);
    gtk_box_pack_start(GTK_BOX(privacy), data->safe_browsing_check, FALSE, FALSE, 0);

    data->usage_stats_check = gtk_check_button_new_with_label("Send usage statistics");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->usage_stats_check), settings.send_usage_stats);
    gtk_box_pack_start(GTK_BOX(privacy), data->usage_stats_check, FALSE, FALSE, 0);

    data->passwords_check = gtk_check_button_new_with_label("Remember passwords");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->passwords_check), settings.remember_passwords);
    gtk_box_pack_start(GTK_BOX(privacy), data->passwords_check, FALSE, FALSE, 0);

    data->autofill_check = gtk_check_button_new_with_label("Autofill forms");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->autofill_check), settings.autofill_enabled);
    gtk_box_pack_start(GTK_BOX(privacy), data->autofill_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // 5. Permissions tab
    // ------------------------------------------------------------------
    GtkWidget *permissions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(permissions), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), permissions, gtk_label_new("Permissions"));

    data->location_check = gtk_check_button_new_with_label("Allow sites to access location");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->location_check), settings.location_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->location_check, FALSE, FALSE, 0);

    data->camera_check = gtk_check_button_new_with_label("Allow sites to access camera");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->camera_check), settings.camera_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->camera_check, FALSE, FALSE, 0);

    data->microphone_check = gtk_check_button_new_with_label("Allow sites to access microphone");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->microphone_check), settings.microphone_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->microphone_check, FALSE, FALSE, 0);

    data->notifications_check = gtk_check_button_new_with_label("Allow notifications");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->notifications_check), settings.notifications_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->notifications_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // 6. Downloads tab
    // ------------------------------------------------------------------
    GtkWidget *downloads = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(downloads), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), downloads, gtk_label_new("Downloads"));

    GtkWidget *download_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(downloads), download_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(download_hbox), gtk_label_new("Download folder:"), FALSE, FALSE, 0);
    data->download_dir_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->download_dir_entry), settings.download_dir);
    gtk_box_pack_start(GTK_BOX(download_hbox), data->download_dir_entry, TRUE, TRUE, 0);
    data->download_dir_button = gtk_button_new_with_label("Browse...");
    g_signal_connect(data->download_dir_button, "clicked", G_CALLBACK(on_download_dir_clicked), data);
    gtk_box_pack_start(GTK_BOX(download_hbox), data->download_dir_button, FALSE, FALSE, 0);

    data->ask_location_check = gtk_check_button_new_with_label("Ask where to save each file before downloading");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->ask_location_check), settings.ask_download_location);
    gtk_box_pack_start(GTK_BOX(downloads), data->ask_location_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // 7. System tab
    // ------------------------------------------------------------------
    GtkWidget *system = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(system), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), system, gtk_label_new("System"));

    data->hw_accel_check = gtk_check_button_new_with_label("Hardware acceleration (may cause crashes)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->hw_accel_check), settings.hardware_acceleration);
    gtk_box_pack_start(GTK_BOX(system), data->hw_accel_check, FALSE, FALSE, 0);

    GtkWidget *ua_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(system), ua_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ua_hbox), gtk_label_new("User Agent:"), FALSE, FALSE, 0);
    data->user_agent_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->user_agent_entry), settings.user_agent);
    gtk_box_pack_start(GTK_BOX(ua_hbox), data->user_agent_entry, TRUE, TRUE, 0);

    data->system_title_bar_check = gtk_check_button_new_with_label("Use system title bar");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->system_title_bar_check), settings.use_system_title_bar);
    gtk_box_pack_start(GTK_BOX(system), data->system_title_bar_check, FALSE, FALSE, 0);

    data->open_background_check = gtk_check_button_new_with_label("Open links in background tabs");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->open_background_check), settings.open_links_in_background);
    gtk_box_pack_start(GTK_BOX(system), data->open_background_check, FALSE, FALSE, 0);

    // Additional appearance options
    data->status_bar_check = gtk_check_button_new_with_label("Show status bar");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->status_bar_check), settings.show_status_bar);
    gtk_box_pack_start(GTK_BOX(appearance), data->status_bar_check, FALSE, FALSE, 0);

    GtkWidget *theme_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), theme_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(theme_hbox), gtk_label_new("Theme Color:"), FALSE, FALSE, 0);
    data->theme_color_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->theme_color_combo), "Default");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->theme_color_combo), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->theme_color_combo), "Sepia");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->theme_color_combo), "Dark");
    gtk_combo_box_set_active(GTK_COMBO_BOX(data->theme_color_combo), settings.theme_color);
    gtk_box_pack_start(GTK_BOX(theme_hbox), data->theme_color_combo, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // 4.5 Privacy & Security (Extended) tab
    // ------------------------------------------------------------------
    GtkWidget *privacy_ext = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(privacy_ext), 10);
    GtkWidget *privacy_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(privacy_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(privacy_scroll), privacy_ext);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), privacy_scroll, gtk_label_new("Privacy+"));

    data->block_third_party_cookies_check = gtk_check_button_new_with_label("Block third-party cookies");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->block_third_party_cookies_check), settings.block_third_party_cookies);
    gtk_box_pack_start(GTK_BOX(privacy_ext), data->block_third_party_cookies_check, FALSE, FALSE, 0);

    GtkWidget *https_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(privacy_ext), https_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(https_hbox), gtk_label_new("HTTPS Only:"), FALSE, FALSE, 0);
    data->https_only_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->https_only_combo), "Off");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->https_only_combo), "All Sites");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->https_only_combo), "Standard Ports");
    gtk_combo_box_set_active(GTK_COMBO_BOX(data->https_only_combo), settings.https_only_mode);
    gtk_box_pack_start(GTK_BOX(https_hbox), data->https_only_combo, FALSE, FALSE, 0);

    data->block_mixed_content_check = gtk_check_button_new_with_label("Block mixed content (HTTP on HTTPS)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->block_mixed_content_check), settings.block_mixed_content);
    gtk_box_pack_start(GTK_BOX(privacy_ext), data->block_mixed_content_check, FALSE, FALSE, 0);

    data->isolate_site_data_check = gtk_check_button_new_with_label("Isolate site data (prevent cross-site tracking)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->isolate_site_data_check), settings.isolate_site_data);
    gtk_box_pack_start(GTK_BOX(privacy_ext), data->isolate_site_data_check, FALSE, FALSE, 0);

    data->block_tracking_scripts_check = gtk_check_button_new_with_label("Block tracking scripts");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->block_tracking_scripts_check), settings.block_tracking_scripts);
    gtk_box_pack_start(GTK_BOX(privacy_ext), data->block_tracking_scripts_check, FALSE, FALSE, 0);

    data->block_cryptomining_check = gtk_check_button_new_with_label("Block cryptomining scripts");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->block_cryptomining_check), settings.block_cryptomining);
    gtk_box_pack_start(GTK_BOX(privacy_ext), data->block_cryptomining_check, FALSE, FALSE, 0);

    data->block_fingerprinting_check = gtk_check_button_new_with_label("Block fingerprinting");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->block_fingerprinting_check), settings.block_fingerprinting);
    gtk_box_pack_start(GTK_BOX(privacy_ext), data->block_fingerprinting_check, FALSE, FALSE, 0);

    GtkWidget *blocked_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(privacy_ext), blocked_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(blocked_hbox), gtk_label_new("Blocked Sites (comma-separated):"), FALSE, FALSE, 0);
    data->blocked_sites_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->blocked_sites_entry), settings.blocked_sites);
    gtk_box_pack_start(GTK_BOX(blocked_hbox), data->blocked_sites_entry, TRUE, TRUE, 0);

    // Additional permissions
    data->clipboard_check = gtk_check_button_new_with_label("Allow sites to access clipboard");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->clipboard_check), settings.clipboard_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->clipboard_check, FALSE, FALSE, 0);

    data->midi_check = gtk_check_button_new_with_label("Allow sites to access MIDI devices");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->midi_check), settings.midi_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->midi_check, FALSE, FALSE, 0);

    data->usb_check = gtk_check_button_new_with_label("Allow sites to access USB devices");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->usb_check), settings.usb_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->usb_check, FALSE, FALSE, 0);

    data->gyroscope_check = gtk_check_button_new_with_label("Allow sites to access gyroscope");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->gyroscope_check), settings.gyroscope_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), data->gyroscope_check, FALSE, FALSE, 0);

    // Additional downloads
    data->show_downloads_check = gtk_check_button_new_with_label("Show downloads when finished");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->show_downloads_check), settings.show_downloads_when_done);
    gtk_box_pack_start(GTK_BOX(downloads), data->show_downloads_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // Tabs & Windows tab
    // ------------------------------------------------------------------
    GtkWidget *tabs_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tabs_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tabs_tab, gtk_label_new("Tabs & Windows"));

    GtkWidget *tab_behavior_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(tabs_tab), tab_behavior_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_behavior_hbox), gtk_label_new("Tab Behavior:"), FALSE, FALSE, 0);
    data->tab_behavior_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->tab_behavior_combo), "Last Tab");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->tab_behavior_combo), "Maintain Tabs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->tab_behavior_combo), "Single Tab");
    gtk_combo_box_set_active(GTK_COMBO_BOX(data->tab_behavior_combo), settings.tab_behavior);
    gtk_box_pack_start(GTK_BOX(tab_behavior_hbox), data->tab_behavior_combo, FALSE, FALSE, 0);

    data->restore_session_check = gtk_check_button_new_with_label("Restore session on startup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->restore_session_check), settings.restore_session_on_startup);
    gtk_box_pack_start(GTK_BOX(tabs_tab), data->restore_session_check, FALSE, FALSE, 0);

    data->tab_groups_check = gtk_check_button_new_with_label("Enable tab groups");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->tab_groups_check), settings.enable_tab_groups);
    gtk_box_pack_start(GTK_BOX(tabs_tab), data->tab_groups_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // Content Settings tab
    // ------------------------------------------------------------------
    GtkWidget *content_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(content_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content_tab, gtk_label_new("Content"));

    data->enable_sound_check = gtk_check_button_new_with_label("Enable sound");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->enable_sound_check), settings.enable_sound);
    gtk_box_pack_start(GTK_BOX(content_tab), data->enable_sound_check, FALSE, FALSE, 0);

    data->autoplay_video_check = gtk_check_button_new_with_label("Allow video autoplay");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->autoplay_video_check), settings.enable_autoplay_video);
    gtk_box_pack_start(GTK_BOX(content_tab), data->autoplay_video_check, FALSE, FALSE, 0);

    data->autoplay_audio_check = gtk_check_button_new_with_label("Allow audio autoplay");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->autoplay_audio_check), settings.enable_autoplay_audio);
    gtk_box_pack_start(GTK_BOX(content_tab), data->autoplay_audio_check, FALSE, FALSE, 0);

    data->pdf_viewer_check = gtk_check_button_new_with_label("Enable built-in PDF viewer");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->pdf_viewer_check), settings.enable_pdf_viewer);
    gtk_box_pack_start(GTK_BOX(content_tab), data->pdf_viewer_check, FALSE, FALSE, 0);

    data->block_ads_check = gtk_check_button_new_with_label("Block ads");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->block_ads_check), settings.block_ads);
    gtk_box_pack_start(GTK_BOX(content_tab), data->block_ads_check, FALSE, FALSE, 0);

    data->reading_mode_check = gtk_check_button_new_with_label("Enable reading mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->reading_mode_check), settings.enable_reading_mode);
    gtk_box_pack_start(GTK_BOX(content_tab), data->reading_mode_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // Accessibility tab
    // ------------------------------------------------------------------
    GtkWidget *access_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(access_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), access_tab, gtk_label_new("Accessibility"));

    data->text_scaled_check = gtk_check_button_new_with_label("Scale text on all sites");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->text_scaled_check), settings.text_scaled);
    gtk_box_pack_start(GTK_BOX(access_tab), data->text_scaled_check, FALSE, FALSE, 0);

    GtkWidget *scale_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(access_tab), scale_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(scale_hbox), gtk_label_new("Text Scale (%):"), FALSE, FALSE, 0);
    data->text_scale_spin = gtk_spin_button_new_with_range(50, 200, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->text_scale_spin), settings.text_scale_percentage);
    gtk_box_pack_start(GTK_BOX(scale_hbox), data->text_scale_spin, FALSE, FALSE, 0);

    data->high_contrast_check = gtk_check_button_new_with_label("High contrast mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->high_contrast_check), settings.high_contrast_mode);
    gtk_box_pack_start(GTK_BOX(access_tab), data->high_contrast_check, FALSE, FALSE, 0);

    data->minimize_ui_check = gtk_check_button_new_with_label("Minimize UI");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->minimize_ui_check), settings.minimize_ui);
    gtk_box_pack_start(GTK_BOX(access_tab), data->minimize_ui_check, FALSE, FALSE, 0);

    data->show_colors_check = gtk_check_button_new_with_label("Show page colors");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->show_colors_check), settings.show_page_colors);
    gtk_box_pack_start(GTK_BOX(access_tab), data->show_colors_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // Language & Translation tab
    // ------------------------------------------------------------------
    GtkWidget *lang_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(lang_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), lang_tab, gtk_label_new("Languages"));

    GtkWidget *lang_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(lang_tab), lang_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(lang_hbox), gtk_label_new("Language:"), FALSE, FALSE, 0);
    data->language_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->language_entry), settings.language);
    gtk_box_pack_start(GTK_BOX(lang_hbox), data->language_entry, FALSE, FALSE, 0);

    data->enable_translation_check = gtk_check_button_new_with_label("Enable page translation");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->enable_translation_check), settings.enable_translation);
    gtk_box_pack_start(GTK_BOX(lang_tab), data->enable_translation_check, FALSE, FALSE, 0);

    GtkWidget *trans_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(lang_tab), trans_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(trans_hbox), gtk_label_new("Translate to:"), FALSE, FALSE, 0);
    data->translation_lang_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->translation_lang_entry), settings.translation_language);
    gtk_box_pack_start(GTK_BOX(trans_hbox), data->translation_lang_entry, FALSE, FALSE, 0);

    // Additional system settings
    data->system_proxy_check = gtk_check_button_new_with_label("Use system proxy settings");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->system_proxy_check), settings.enable_system_proxy);
    gtk_box_pack_start(GTK_BOX(system), data->system_proxy_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // Cache & Storage tab
    // ------------------------------------------------------------------
    GtkWidget *cache_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(cache_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cache_tab, gtk_label_new("Cache & Storage"));

    data->cache_enabled_check = gtk_check_button_new_with_label("Enable cache");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->cache_enabled_check), settings.cache_enabled);
    gtk_box_pack_start(GTK_BOX(cache_tab), data->cache_enabled_check, FALSE, FALSE, 0);

    GtkWidget *cache_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(cache_tab), cache_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cache_hbox), gtk_label_new("Cache Size:"), FALSE, FALSE, 0);
    data->cache_size_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->cache_size_combo), "Unlimited");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->cache_size_combo), "100 MB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->cache_size_combo), "500 MB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->cache_size_combo), "1 GB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->cache_size_combo), "Disabled");
    gtk_combo_box_set_active(GTK_COMBO_BOX(data->cache_size_combo), settings.cache_size);
    gtk_box_pack_start(GTK_BOX(cache_hbox), data->cache_size_combo, FALSE, FALSE, 0);

    GtkWidget *cookie_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(cache_tab), cookie_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cookie_hbox), gtk_label_new("Cookie expiration (days, 0=session):"), FALSE, FALSE, 0);
    data->cookie_expiry_spin = gtk_spin_button_new_with_range(0, 365, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->cookie_expiry_spin), settings.cookie_expiration_days);
    gtk_box_pack_start(GTK_BOX(cookie_hbox), data->cookie_expiry_spin, FALSE, FALSE, 0);

    data->offline_content_check = gtk_check_button_new_with_label("Store content for offline use");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->offline_content_check), settings.offline_content_enabled);
    gtk_box_pack_start(GTK_BOX(cache_tab), data->offline_content_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // History & Data tab
    // ------------------------------------------------------------------
    GtkWidget *history_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(history_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), history_tab, gtk_label_new("History & Data"));

    data->history_check = gtk_check_button_new_with_label("Remember browsing history");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->history_check), settings.remember_history);
    gtk_box_pack_start(GTK_BOX(history_tab), data->history_check, FALSE, FALSE, 0);

    // Enhanced history tab
    data->remember_forms_check = gtk_check_button_new_with_label("Remember form entries");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->remember_forms_check), settings.remember_form_entries);
    gtk_box_pack_start(GTK_BOX(history_tab), data->remember_forms_check, FALSE, FALSE, 0);

    GtkWidget *history_days_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(history_tab), history_days_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(history_days_hbox), gtk_label_new("Keep history for (days, 0=forever):"), FALSE, FALSE, 0);
    data->history_days_spin = gtk_spin_button_new_with_range(0, 365, 30);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->history_days_spin), settings.history_days_to_keep);
    gtk_box_pack_start(GTK_BOX(history_days_hbox), data->history_days_spin, FALSE, FALSE, 0);

    data->clear_cache_exit_check = gtk_check_button_new_with_label("Clear cache on exit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->clear_cache_exit_check), settings.clear_cache_on_exit);
    gtk_box_pack_start(GTK_BOX(history_tab), data->clear_cache_exit_check, FALSE, FALSE, 0);

    data->clear_cookies_exit_check = gtk_check_button_new_with_label("Clear cookies on exit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->clear_cookies_exit_check), settings.clear_cookies_on_exit);
    gtk_box_pack_start(GTK_BOX(history_tab), data->clear_cookies_exit_check, FALSE, FALSE, 0);

    data->clear_history_exit_check = gtk_check_button_new_with_label("Clear history on exit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->clear_history_exit_check), settings.clear_history_on_exit);
    gtk_box_pack_start(GTK_BOX(history_tab), data->clear_history_exit_check, FALSE, FALSE, 0);

    // ------------------------------------------------------------------
    // Advanced tab
    // ------------------------------------------------------------------
    GtkWidget *advanced = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(advanced), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), advanced, gtk_label_new("Advanced"));

    GtkWidget *startup_cmd_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(advanced), startup_cmd_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(startup_cmd_hbox), gtk_label_new("Startup Command:"), FALSE, FALSE, 0);
    data->startup_command_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->startup_command_entry), settings.startup_command);
    gtk_box_pack_start(GTK_BOX(startup_cmd_hbox), data->startup_command_entry, TRUE, TRUE, 0);

    data->dev_tools_check = gtk_check_button_new_with_label("Enable developer tools");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->dev_tools_check), settings.enable_developer_tools);
    gtk_box_pack_start(GTK_BOX(advanced), data->dev_tools_check, FALSE, FALSE, 0);

    data->debugging_check = gtk_check_button_new_with_label("Enable debugging (remote debugging protocol)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->debugging_check), settings.enable_debugging);
    gtk_box_pack_start(GTK_BOX(advanced), data->debugging_check, FALSE, FALSE, 0);

    // Show everything and ensure the dialog gets focus
    gtk_widget_show_all(dialog);
    gtk_window_present(GTK_WINDOW(dialog)); // Ensure the dialog gets focus

    g_signal_connect(dialog, "response", G_CALLBACK(on_settings_response), data);
}

// Close tab callback for settings tab
static void on_settings_close_tab_clicked(GtkButton *button, BrowserWindow *browser)
{
    GtkWidget *tab_child = g_object_get_data(G_OBJECT(button), "tab-child");
    if (tab_child) {
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(browser->notebook), tab_child);
        if (page_num != -1) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), page_num);
        }
    }
}

// Settings Tab - opens settings inside the browser as a tab
void show_settings_tab(BrowserWindow *browser)
{
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Settings</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), title, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(scrolled), notebook);
    
    // ========== GENERAL TAB ==========
    GtkWidget *general_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(general_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *general = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(general), 10);
    gtk_container_add(GTK_CONTAINER(general_scroll), general);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_scroll, gtk_label_new("General"));
    
    GtkWidget *home_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), home_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(home_hbox), gtk_label_new("Home page:"), FALSE, FALSE, 0);
    GtkWidget *home_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(home_entry), settings.home_page);
    gtk_box_pack_start(GTK_BOX(home_hbox), home_entry, TRUE, TRUE, 0);
    
    // Search Engine
    GtkWidget *search_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), search_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_hbox), gtk_label_new("Search Engine:"), FALSE, FALSE, 0);
    GtkWidget *search_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Bing");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Yahoo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Custom");
    gtk_combo_box_set_active(GTK_COMBO_BOX(search_combo), settings.search_engine);
    gtk_box_pack_start(GTK_BOX(search_hbox), search_combo, FALSE, FALSE, 0);
    
    // Custom Search URL
    GtkWidget *custom_search_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), custom_search_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_search_hbox), gtk_label_new("Custom Search URL:"), FALSE, FALSE, 0);
    GtkWidget *custom_search_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(custom_search_entry), settings.custom_search_url);
    gtk_box_pack_start(GTK_BOX(custom_search_hbox), custom_search_entry, TRUE, TRUE, 0);
    
    // Startup Behavior
    GtkWidget *startup_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), startup_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(startup_hbox), gtk_label_new("Startup:"), FALSE, FALSE, 0);
    GtkWidget *startup_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(startup_combo), "New Tab Page");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(startup_combo), "Continue");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(startup_combo), "Home Page");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(startup_combo), "Specific Pages");
    gtk_combo_box_set_active(GTK_COMBO_BOX(startup_combo), settings.startup_behavior);
    gtk_box_pack_start(GTK_BOX(startup_hbox), startup_combo, FALSE, FALSE, 0);
    
    // Language
    GtkWidget *lang_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), lang_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(lang_hbox), gtk_label_new("Language:"), FALSE, FALSE, 0);
    GtkWidget *lang_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(lang_entry), settings.language);
    gtk_box_pack_start(GTK_BOX(lang_hbox), lang_entry, FALSE, FALSE, 0);
    
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(general), sep1, FALSE, FALSE, 0);
    
    GtkWidget *open_links_bg = gtk_check_button_new_with_label("Open links in background");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(open_links_bg), settings.open_links_in_background);
    gtk_box_pack_start(GTK_BOX(general), open_links_bg, FALSE, FALSE, 0);
    
    GtkWidget *restore_session = gtk_check_button_new_with_label("Restore session on startup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(restore_session), settings.restore_session_on_startup);
    gtk_box_pack_start(GTK_BOX(general), restore_session, FALSE, FALSE, 0);
    
    // ========== APPEARANCE TAB ==========
    GtkWidget *appearance_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(appearance_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *appearance = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(appearance), 10);
    gtk_container_add(GTK_CONTAINER(appearance_scroll), appearance);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), appearance_scroll, gtk_label_new("Appearance"));
    
    GtkWidget *dark_mode_check = gtk_check_button_new_with_label("Dark mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dark_mode_check), settings.dark_mode);
    gtk_box_pack_start(GTK_BOX(appearance), dark_mode_check, FALSE, FALSE, 0);
    
    GtkWidget *high_contrast_check = gtk_check_button_new_with_label("High contrast mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(high_contrast_check), settings.high_contrast_mode);
    gtk_box_pack_start(GTK_BOX(appearance), high_contrast_check, FALSE, FALSE, 0);
    
    GtkWidget *font_family_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), font_family_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(font_family_hbox), gtk_label_new("Font family:"), FALSE, FALSE, 0);
    GtkWidget *font_family_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(font_family_entry), settings.font_family);
    gtk_box_pack_start(GTK_BOX(font_family_hbox), font_family_entry, TRUE, TRUE, 0);
    
    GtkWidget *font_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), font_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(font_hbox), gtk_label_new("Font size:"), FALSE, FALSE, 0);
    GtkWidget *font_size_spin = gtk_spin_button_new_with_range(6, 72, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(font_size_spin), settings.font_size);
    gtk_box_pack_start(GTK_BOX(font_hbox), font_size_spin, FALSE, FALSE, 0);
    
    GtkWidget *zoom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), zoom_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_hbox), gtk_label_new("Zoom level:"), FALSE, FALSE, 0);
    GtkWidget *zoom_spin = gtk_spin_button_new_with_range(50, 300, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_spin), settings.zoom_level);
    gtk_box_pack_start(GTK_BOX(zoom_hbox), zoom_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_hbox), gtk_label_new("%"), FALSE, FALSE, 0);
    
    GtkWidget *text_scale_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(appearance), text_scale_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_scale_hbox), gtk_label_new("Text scale:"), FALSE, FALSE, 0);
    GtkWidget *text_scale_spin = gtk_spin_button_new_with_range(50, 200, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(text_scale_spin), settings.text_scale_percentage);
    gtk_box_pack_start(GTK_BOX(text_scale_hbox), text_scale_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_scale_hbox), gtk_label_new("%"), FALSE, FALSE, 0);
    
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(appearance), sep2, FALSE, FALSE, 0);
    
    GtkWidget *bookmarks_bar = gtk_check_button_new_with_label("Show bookmarks bar");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bookmarks_bar), settings.show_bookmarks_bar);
    gtk_box_pack_start(GTK_BOX(appearance), bookmarks_bar, FALSE, FALSE, 0);
    
    GtkWidget *home_btn = gtk_check_button_new_with_label("Show home button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(home_btn), settings.show_home_button);
    gtk_box_pack_start(GTK_BOX(appearance), home_btn, FALSE, FALSE, 0);
    
    GtkWidget *status_bar = gtk_check_button_new_with_label("Show status bar");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(status_bar), settings.show_status_bar);
    gtk_box_pack_start(GTK_BOX(appearance), status_bar, FALSE, FALSE, 0);
    
    GtkWidget *minimize_ui = gtk_check_button_new_with_label("Minimize UI");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(minimize_ui), settings.minimize_ui);
    gtk_box_pack_start(GTK_BOX(appearance), minimize_ui, FALSE, FALSE, 0);
    
    // ========== PRIVACY & SECURITY TAB ==========
    GtkWidget *privacy_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(privacy_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *privacy = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(privacy), 10);
    gtk_container_add(GTK_CONTAINER(privacy_scroll), privacy);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), privacy_scroll, gtk_label_new("Privacy & Security"));
    
    GtkWidget *js_check = gtk_check_button_new_with_label("Enable JavaScript");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(js_check), settings.enable_javascript);
    gtk_box_pack_start(GTK_BOX(privacy), js_check, FALSE, FALSE, 0);
    
    GtkWidget *cookies_check = gtk_check_button_new_with_label("Enable Cookies");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cookies_check), settings.enable_cookies);
    gtk_box_pack_start(GTK_BOX(privacy), cookies_check, FALSE, FALSE, 0);
    
    GtkWidget *third_party_cookies = gtk_check_button_new_with_label("Block third-party cookies");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(third_party_cookies), settings.block_third_party_cookies);
    gtk_box_pack_start(GTK_BOX(privacy), third_party_cookies, FALSE, FALSE, 0);
    
    GtkWidget *images_check = gtk_check_button_new_with_label("Load Images");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(images_check), settings.enable_images);
    gtk_box_pack_start(GTK_BOX(privacy), images_check, FALSE, FALSE, 0);
    
    GtkWidget *popups_check = gtk_check_button_new_with_label("Block Pop-up Windows");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(popups_check), !settings.enable_popups);
    gtk_box_pack_start(GTK_BOX(privacy), popups_check, FALSE, FALSE, 0);
    
    GtkWidget *plugins_check = gtk_check_button_new_with_label("Enable Plugins");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(plugins_check), settings.enable_plugins);
    gtk_box_pack_start(GTK_BOX(privacy), plugins_check, FALSE, FALSE, 0);
    
    GtkWidget *safe_browsing = gtk_check_button_new_with_label("Safe browsing enabled");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(safe_browsing), settings.safe_browsing_enabled);
    gtk_box_pack_start(GTK_BOX(privacy), safe_browsing, FALSE, FALSE, 0);
    
    GtkWidget *dnt_check = gtk_check_button_new_with_label("Do Not Track");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dnt_check), settings.do_not_track);
    gtk_box_pack_start(GTK_BOX(privacy), dnt_check, FALSE, FALSE, 0);
    
    GtkWidget *stats_check = gtk_check_button_new_with_label("Send usage statistics");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stats_check), settings.send_usage_stats);
    gtk_box_pack_start(GTK_BOX(privacy), stats_check, FALSE, FALSE, 0);
    
    GtkWidget *predictive_check = gtk_check_button_new_with_label("Predictive search");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(predictive_check), settings.predictive_search);
    gtk_box_pack_start(GTK_BOX(privacy), predictive_check, FALSE, FALSE, 0);
    
    GtkWidget *remember_pwd = gtk_check_button_new_with_label("Remember passwords");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remember_pwd), settings.remember_passwords);
    gtk_box_pack_start(GTK_BOX(privacy), remember_pwd, FALSE, FALSE, 0);
    
    GtkWidget *autofill_check = gtk_check_button_new_with_label("Autofill forms");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autofill_check), settings.autofill_enabled);
    gtk_box_pack_start(GTK_BOX(privacy), autofill_check, FALSE, FALSE, 0);
    
    GtkWidget *https_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(privacy), https_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(https_hbox), gtk_label_new("HTTPS Only:"), FALSE, FALSE, 0);
    GtkWidget *https_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(https_combo), "Off");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(https_combo), "All Sites");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(https_combo), "Standard Ports");
    gtk_combo_box_set_active(GTK_COMBO_BOX(https_combo), settings.https_only_mode);
    gtk_box_pack_start(GTK_BOX(https_hbox), https_combo, FALSE, FALSE, 0);
    
    GtkWidget *mixed_content = gtk_check_button_new_with_label("Block mixed content");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mixed_content), settings.block_mixed_content);
    gtk_box_pack_start(GTK_BOX(privacy), mixed_content, FALSE, FALSE, 0);
    
    GtkWidget *isolate_site = gtk_check_button_new_with_label("Isolate site data");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(isolate_site), settings.isolate_site_data);
    gtk_box_pack_start(GTK_BOX(privacy), isolate_site, FALSE, FALSE, 0);
    
    GtkWidget *sep_tracking = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(privacy), sep_tracking, FALSE, FALSE, 0);
    
    GtkWidget *block_tracking = gtk_check_button_new_with_label("Block tracking scripts");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(block_tracking), settings.block_tracking_scripts);
    gtk_box_pack_start(GTK_BOX(privacy), block_tracking, FALSE, FALSE, 0);
    
    GtkWidget *block_crypto = gtk_check_button_new_with_label("Block cryptomining");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(block_crypto), settings.block_cryptomining);
    gtk_box_pack_start(GTK_BOX(privacy), block_crypto, FALSE, FALSE, 0);
    
    GtkWidget *block_fingerprint = gtk_check_button_new_with_label("Block fingerprinting");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(block_fingerprint), settings.block_fingerprinting);
    gtk_box_pack_start(GTK_BOX(privacy), block_fingerprint, FALSE, FALSE, 0);
    
    // ========== PERMISSIONS TAB ==========
    GtkWidget *perms_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(perms_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *permissions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(permissions), 10);
    gtk_container_add(GTK_CONTAINER(perms_scroll), permissions);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), perms_scroll, gtk_label_new("Permissions"));
    
    GtkWidget *location_check = gtk_check_button_new_with_label("Allow Location access");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(location_check), settings.location_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), location_check, FALSE, FALSE, 0);
    
    GtkWidget *camera_check = gtk_check_button_new_with_label("Allow Camera access");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(camera_check), settings.camera_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), camera_check, FALSE, FALSE, 0);
    
    GtkWidget *mic_check = gtk_check_button_new_with_label("Allow Microphone access");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mic_check), settings.microphone_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), mic_check, FALSE, FALSE, 0);
    
    GtkWidget *notify_check = gtk_check_button_new_with_label("Allow Notifications");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(notify_check), settings.notifications_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), notify_check, FALSE, FALSE, 0);
    
    GtkWidget *clipboard_check = gtk_check_button_new_with_label("Allow Clipboard access");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clipboard_check), settings.clipboard_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), clipboard_check, FALSE, FALSE, 0);
    
    GtkWidget *midi_check = gtk_check_button_new_with_label("Allow MIDI access");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(midi_check), settings.midi_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), midi_check, FALSE, FALSE, 0);
    
    GtkWidget *usb_check = gtk_check_button_new_with_label("Allow USB access");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(usb_check), settings.usb_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), usb_check, FALSE, FALSE, 0);
    
    GtkWidget *gyro_check = gtk_check_button_new_with_label("Allow Gyroscope access");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gyro_check), settings.gyroscope_enabled);
    gtk_box_pack_start(GTK_BOX(permissions), gyro_check, FALSE, FALSE, 0);
    
    // ========== DOWNLOADS TAB ==========
    GtkWidget *download_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(download_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *downloads = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(downloads), 10);
    gtk_container_add(GTK_CONTAINER(download_scroll), downloads);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), download_scroll, gtk_label_new("Downloads"));
    
    GtkWidget *download_dir_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(downloads), download_dir_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(download_dir_hbox), gtk_label_new("Download directory:"), FALSE, FALSE, 0);
    GtkWidget *download_dir_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(download_dir_entry), settings.download_dir);
    gtk_box_pack_start(GTK_BOX(download_dir_hbox), download_dir_entry, TRUE, TRUE, 0);
    
    GtkWidget *ask_location = gtk_check_button_new_with_label("Ask where to save");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ask_location), settings.ask_download_location);
    gtk_box_pack_start(GTK_BOX(downloads), ask_location, FALSE, FALSE, 0);
    
    GtkWidget *show_when_done = gtk_check_button_new_with_label("Show downloads when done");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_when_done), settings.show_downloads_when_done);
    gtk_box_pack_start(GTK_BOX(downloads), show_when_done, FALSE, FALSE, 0);
    
    // ========== CONTENT TAB ==========
    GtkWidget *content_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(content_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);
    gtk_container_add(GTK_CONTAINER(content_scroll), content);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content_scroll, gtk_label_new("Content"));
    
    GtkWidget *enable_sound = gtk_check_button_new_with_label("Enable sound");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_sound), settings.enable_sound);
    gtk_box_pack_start(GTK_BOX(content), enable_sound, FALSE, FALSE, 0);
    
    GtkWidget *autoplay_video = gtk_check_button_new_with_label("Enable autoplay (video)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autoplay_video), settings.enable_autoplay_video);
    gtk_box_pack_start(GTK_BOX(content), autoplay_video, FALSE, FALSE, 0);
    
    GtkWidget *autoplay_audio = gtk_check_button_new_with_label("Enable autoplay (audio)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autoplay_audio), settings.enable_autoplay_audio);
    gtk_box_pack_start(GTK_BOX(content), autoplay_audio, FALSE, FALSE, 0);
    
    GtkWidget *pdf_viewer = gtk_check_button_new_with_label("Built-in PDF viewer");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pdf_viewer), settings.enable_pdf_viewer);
    gtk_box_pack_start(GTK_BOX(content), pdf_viewer, FALSE, FALSE, 0);
    
    GtkWidget *block_ads = gtk_check_button_new_with_label("Block ads");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(block_ads), settings.block_ads);
    gtk_box_pack_start(GTK_BOX(content), block_ads, FALSE, FALSE, 0);
    
    GtkWidget *reading_mode = gtk_check_button_new_with_label("Reading mode available");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reading_mode), settings.enable_reading_mode);
    gtk_box_pack_start(GTK_BOX(content), reading_mode, FALSE, FALSE, 0);
    
    // ========== SYSTEM TAB ==========
    GtkWidget *system_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(system_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *system = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(system), 10);
    gtk_container_add(GTK_CONTAINER(system_scroll), system);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), system_scroll, gtk_label_new("System"));
    
    GtkWidget *hardware_accel = gtk_check_button_new_with_label("Hardware acceleration");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hardware_accel), settings.hardware_acceleration);
    gtk_box_pack_start(GTK_BOX(system), hardware_accel, FALSE, FALSE, 0);
    
    GtkWidget *sys_titlebar = gtk_check_button_new_with_label("Use system title bar");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sys_titlebar), settings.use_system_title_bar);
    gtk_box_pack_start(GTK_BOX(system), sys_titlebar, FALSE, FALSE, 0);
    
    GtkWidget *sys_proxy = gtk_check_button_new_with_label("Use system proxy");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sys_proxy), settings.enable_system_proxy);
    gtk_box_pack_start(GTK_BOX(system), sys_proxy, FALSE, FALSE, 0);
    
    GtkWidget *user_agent_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(system), user_agent_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(user_agent_hbox), gtk_label_new("User Agent:"), FALSE, FALSE, 0);
    GtkWidget *user_agent_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(user_agent_entry), settings.user_agent);
    gtk_box_pack_start(GTK_BOX(user_agent_hbox), user_agent_entry, TRUE, TRUE, 0);
    
    // ========== CACHE & STORAGE TAB ==========
    GtkWidget *cache_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cache_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *cache_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(cache_tab), 10);
    gtk_container_add(GTK_CONTAINER(cache_scroll), cache_tab);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cache_scroll, gtk_label_new("Cache & Storage"));
    
    GtkWidget *cache_enabled = gtk_check_button_new_with_label("Cache enabled");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cache_enabled), settings.cache_enabled);
    gtk_box_pack_start(GTK_BOX(cache_tab), cache_enabled, FALSE, FALSE, 0);
    
    GtkWidget *cache_size_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(cache_tab), cache_size_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cache_size_hbox), gtk_label_new("Cache size:"), FALSE, FALSE, 0);
    GtkWidget *cache_size_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cache_size_combo), "Unlimited");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cache_size_combo), "100 MB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cache_size_combo), "500 MB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cache_size_combo), "1 GB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cache_size_combo), "Disabled");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cache_size_combo), settings.cache_size);
    gtk_box_pack_start(GTK_BOX(cache_size_hbox), cache_size_combo, FALSE, FALSE, 0);
    
    GtkWidget *cookie_exp_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(cache_tab), cookie_exp_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cookie_exp_hbox), gtk_label_new("Cookie expiration (days):"), FALSE, FALSE, 0);
    GtkWidget *cookie_exp_spin = gtk_spin_button_new_with_range(0, 365, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(cookie_exp_spin), settings.cookie_expiration_days);
    gtk_box_pack_start(GTK_BOX(cookie_exp_hbox), cookie_exp_spin, FALSE, FALSE, 0);
    
    GtkWidget *offline_content = gtk_check_button_new_with_label("Offline content enabled");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(offline_content), settings.offline_content_enabled);
    gtk_box_pack_start(GTK_BOX(cache_tab), offline_content, FALSE, FALSE, 0);
    
    // ========== HISTORY & DATA TAB ==========
    GtkWidget *history_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(history_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *history_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(history_tab), 10);
    gtk_container_add(GTK_CONTAINER(history_scroll), history_tab);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), history_scroll, gtk_label_new("History & Data"));
    
    GtkWidget *remember_hst = gtk_check_button_new_with_label("Remember history");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remember_hst), settings.remember_history);
    gtk_box_pack_start(GTK_BOX(history_tab), remember_hst, FALSE, FALSE, 0);
    
    GtkWidget *remember_forms = gtk_check_button_new_with_label("Remember form entries");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remember_forms), settings.remember_form_entries);
    gtk_box_pack_start(GTK_BOX(history_tab), remember_forms, FALSE, FALSE, 0);
    
    GtkWidget *history_days_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(history_tab), history_days_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(history_days_hbox), gtk_label_new("History retention (days):"), FALSE, FALSE, 0);
    GtkWidget *history_days_spin = gtk_spin_button_new_with_range(0, 9999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(history_days_spin), settings.history_days_to_keep);
    gtk_box_pack_start(GTK_BOX(history_days_hbox), history_days_spin, FALSE, FALSE, 0);
    
    GtkWidget *clear_cache_exit = gtk_check_button_new_with_label("Clear cache on exit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clear_cache_exit), settings.clear_cache_on_exit);
    gtk_box_pack_start(GTK_BOX(history_tab), clear_cache_exit, FALSE, FALSE, 0);
    
    GtkWidget *clear_cookies_exit = gtk_check_button_new_with_label("Clear cookies on exit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clear_cookies_exit), settings.clear_cookies_on_exit);
    gtk_box_pack_start(GTK_BOX(history_tab), clear_cookies_exit, FALSE, FALSE, 0);
    
    GtkWidget *clear_history_exit = gtk_check_button_new_with_label("Clear history on exit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clear_history_exit), settings.clear_history_on_exit);
    gtk_box_pack_start(GTK_BOX(history_tab), clear_history_exit, FALSE, FALSE, 0);
    
    // ========== ADVANCED TAB ==========
    GtkWidget *adv_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(adv_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *advanced = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(advanced), 10);
    gtk_container_add(GTK_CONTAINER(adv_scroll), advanced);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), adv_scroll, gtk_label_new("Advanced"));
    
    GtkWidget *dev_tools = gtk_check_button_new_with_label("Enable developer tools");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dev_tools), settings.enable_developer_tools);
    gtk_box_pack_start(GTK_BOX(advanced), dev_tools, FALSE, FALSE, 0);
    
    GtkWidget *enable_debug = gtk_check_button_new_with_label("Enable debugging");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_debug), settings.enable_debugging);
    gtk_box_pack_start(GTK_BOX(advanced), enable_debug, FALSE, FALSE, 0);
    
    GtkWidget *enable_translation = gtk_check_button_new_with_label("Enable translation");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_translation), settings.enable_translation);
    gtk_box_pack_start(GTK_BOX(advanced), enable_translation, FALSE, FALSE, 0);
    
    gtk_widget_show_all(tab_content);
    
    // Create tab label with close button
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Settings");
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(close_btn, "Close tab");
    
    gtk_box_pack_start(GTK_BOX(tab_box), tab_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_box);
    
    g_object_set_data_full(G_OBJECT(close_btn), "tab-child", tab_content, NULL);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_settings_close_tab_clicked), browser);
    
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(browser->notebook), tab_content, tab_box);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), page_num);
}