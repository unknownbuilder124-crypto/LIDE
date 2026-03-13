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

    // Permissions
    settings.location_enabled = FALSE;
    settings.camera_enabled = FALSE;
    settings.microphone_enabled = FALSE;
    settings.notifications_enabled = TRUE;

    // Downloads
    settings.download_dir = g_strdup(default_download_dir);
    settings.ask_download_location = TRUE;

    // System
    settings.hardware_acceleration = FALSE;
    settings.user_agent = g_strdup(default_user_agent);
    settings.use_system_title_bar = TRUE;
    settings.open_links_in_background = FALSE;

    // History
    settings.remember_history = TRUE;
}

// Free any allocated strings inside settings (called before overwriting)
static void free_settings_strings(void)
{
    g_free(settings.home_page);
    g_free(settings.custom_search_url);
    g_free(settings.download_dir);
    g_free(settings.user_agent);
    g_free(settings.font_family);
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
        else if (strcmp(key, "download_dir") == 0) {
            g_free(settings.download_dir);
            settings.download_dir = g_strdup(value);
        }
        else if (strcmp(key, "ask_download_location") == 0) {
            settings.ask_download_location = atoi(value) != 0;
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
        else if (strcmp(key, "remember_history") == 0) {
            settings.remember_history = atoi(value) != 0;
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

    // Permissions
    fprintf(f, "location_enabled=%d\n", settings.location_enabled);
    fprintf(f, "camera_enabled=%d\n", settings.camera_enabled);
    fprintf(f, "microphone_enabled=%d\n", settings.microphone_enabled);
    fprintf(f, "notifications_enabled=%d\n", settings.notifications_enabled);

    // Downloads
    fprintf(f, "download_dir=%s\n", settings.download_dir);
    fprintf(f, "ask_download_location=%d\n", settings.ask_download_location);

    // System
    fprintf(f, "hardware_acceleration=%d\n", settings.hardware_acceleration);
    fprintf(f, "user_agent=%s\n", settings.user_agent);
    fprintf(f, "use_system_title_bar=%d\n", settings.use_system_title_bar);
    fprintf(f, "open_links_in_background=%d\n", settings.open_links_in_background);

    // History
    fprintf(f, "remember_history=%d\n", settings.remember_history);

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

    // Permissions
    GtkWidget *location_check;
    GtkWidget *camera_check;
    GtkWidget *microphone_check;
    GtkWidget *notifications_check;

    // Downloads
    GtkWidget *download_dir_entry;
    GtkWidget *download_dir_button;
    GtkWidget *ask_location_check;

    // System
    GtkWidget *hw_accel_check;
    GtkWidget *user_agent_entry;
    GtkWidget *system_title_bar_check;
    GtkWidget *open_background_check;

    // History
    GtkWidget *history_check;
    GtkWidget *clear_history_btn;

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

        // ----- Permissions -----
        settings.location_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->location_check));
        settings.camera_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->camera_check));
        settings.microphone_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->microphone_check));
        settings.notifications_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->notifications_check));

        // ----- Downloads -----
        g_free(settings.download_dir);
        settings.download_dir = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->download_dir_entry)));
        settings.ask_download_location = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->ask_location_check));

        // ----- System -----
        settings.hardware_acceleration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->hw_accel_check));
        g_free(settings.user_agent);
        settings.user_agent = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->user_agent_entry)));
        settings.use_system_title_bar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->system_title_bar_check));
        settings.open_links_in_background = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->open_background_check));

        // ----- History -----
        settings.remember_history = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->history_check));

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

    // ------------------------------------------------------------------
    // 8. History tab
    // ------------------------------------------------------------------
    GtkWidget *history_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(history_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), history_tab, gtk_label_new("History"));

    data->history_check = gtk_check_button_new_with_label("Remember browsing history");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->history_check), settings.remember_history);
    gtk_box_pack_start(GTK_BOX(history_tab), data->history_check, FALSE, FALSE, 0);

    data->clear_history_btn = gtk_button_new_with_label("Clear History");
    g_signal_connect(data->clear_history_btn, "clicked", G_CALLBACK(clear_history), browser);
    gtk_box_pack_start(GTK_BOX(history_tab), data->clear_history_btn, FALSE, FALSE, 0);

    // Show everything and ensure the dialog gets focus
    gtk_widget_show_all(dialog);
    gtk_window_present(GTK_WINDOW(dialog)); // Ensure the dialog gets focus

    g_signal_connect(dialog, "response", G_CALLBACK(on_settings_response), data);
}