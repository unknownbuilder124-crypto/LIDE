#include "app_menu.h"
#include "bookmarks.h"
#include "history.h"
#include "downloads.h"
#include "passwords.h"
#include "extensions.h"
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// External references (declared in voidfox.h)
extern void show_bookmarks_tab(BrowserWindow *browser);
extern void show_history_tab(BrowserWindow *browser);
extern void show_downloads_tab(BrowserWindow *browser);
extern void show_passwords_tab(BrowserWindow *browser);
extern void show_extensions_tab(BrowserWindow *browser);

// Find in page dialog data
typedef struct {
    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget *case_sensitive_check;
    GtkWidget *wrap_check;
    WebKitWebView *web_view;
    WebKitFindController *find_controller;
} FindDialogData;

// Find in page callbacks
static void find_next(GtkButton *button, FindDialogData *data)
{
    (void)button;
    const char *text = gtk_entry_get_text(GTK_ENTRY(data->entry));
    WebKitFindOptions options = WEBKIT_FIND_OPTIONS_WRAP_AROUND;
    
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->case_sensitive_check))) {
        options |= WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;
    }
    
    if (text && *text) {
        webkit_find_controller_search(data->find_controller, text, options, G_MAXUINT);
    }
}

static void find_previous(GtkButton *button, FindDialogData *data)
{
    (void)button;
    const char *text = gtk_entry_get_text(GTK_ENTRY(data->entry));
    WebKitFindOptions options = WEBKIT_FIND_OPTIONS_WRAP_AROUND | WEBKIT_FIND_OPTIONS_BACKWARDS;
    
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->case_sensitive_check))) {
        options |= WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;
    }
    
    if (text && *text) {
        webkit_find_controller_search(data->find_controller, text, options, G_MAXUINT);
    }
}

static void find_dialog_response(GtkDialog *dialog, gint response_id, FindDialogData *data)
{
    if (response_id == GTK_RESPONSE_CLOSE || response_id == GTK_RESPONSE_DELETE_EVENT) {
        // Stop highlighting when closing
        webkit_find_controller_search_finish(data->find_controller);
        gtk_widget_destroy(GTK_WIDGET(dialog));
        g_free(data);
    }
}

static void find_entry_activate(GtkEntry *entry, FindDialogData *data)
{
    (void)entry;
    find_next(NULL, data);
}

// Callback for menu items
static void on_new_window_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    // Launch a new browser window
    pid_t pid = fork();
    if (pid == 0) {
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    }
}

static void on_new_private_window_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    // Launch a new private window
    pid_t pid = fork();
    if (pid == 0) {
        execl("./voidfox", "voidfox", "--private", NULL);
        exit(0);
    }
}

static void on_bookmarks_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    show_bookmarks_tab(browser);
}

static void on_history_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    show_history_tab(browser);
}

static void on_downloads_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    show_downloads_tab(browser);
}

static void on_passwords_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    show_passwords_tab(browser);
}

static void on_extensions_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    show_extensions_tab(browser);
}

static void on_print_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        // Use non-deprecated function
        webkit_web_view_evaluate_javascript(tab->web_view, "window.print();", -1, NULL, NULL, NULL, NULL, NULL);
    }
}

static void on_find_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (!tab || !tab->web_view) return;
    
    // Create find dialog
    FindDialogData *data = g_new(FindDialogData, 1);
    data->web_view = tab->web_view;
    data->find_controller = webkit_web_view_get_find_controller(tab->web_view);
    
    data->dialog = gtk_dialog_new_with_buttons("Find in Page",
                                               GTK_WINDOW(browser->window),
                                               GTK_DIALOG_MODAL,
                                               "_Close", GTK_RESPONSE_CLOSE,
                                               NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(data->dialog), 450, 200);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(data->dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);
    
    // Search entry
    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    
    GtkWidget *search_label = gtk_label_new("Search:");
    gtk_box_pack_start(GTK_BOX(hbox1), search_label, FALSE, FALSE, 0);
    
    data->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(data->entry), "Enter text to find...");
    gtk_box_pack_start(GTK_BOX(hbox1), data->entry, TRUE, TRUE, 0);
    
    // Options
    GtkWidget *options_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), options_box, FALSE, FALSE, 0);
    
    data->case_sensitive_check = gtk_check_button_new_with_label("Case sensitive");
    gtk_box_pack_start(GTK_BOX(options_box), data->case_sensitive_check, FALSE, FALSE, 0);
    
    data->wrap_check = gtk_check_button_new_with_label("Wrap around");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->wrap_check), TRUE);
    gtk_box_pack_start(GTK_BOX(options_box), data->wrap_check, FALSE, FALSE, 0);
    
    // Navigation buttons
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    
    GtkWidget *prev_btn = gtk_button_new_with_label("Previous");
    GtkWidget *next_btn = gtk_button_new_with_label("Next");
    gtk_box_pack_start(GTK_BOX(hbox2), prev_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), next_btn, TRUE, TRUE, 0);
    
    // Connect signals
    g_signal_connect(next_btn, "clicked", G_CALLBACK(find_next), data);
    g_signal_connect(prev_btn, "clicked", G_CALLBACK(find_previous), data);
    g_signal_connect(data->entry, "activate", G_CALLBACK(find_entry_activate), data);
    
    g_signal_connect(data->dialog, "response", G_CALLBACK(find_dialog_response), data);
    
    gtk_widget_show_all(data->dialog);
}

static void on_zoom_in_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        webkit_web_view_set_zoom_level(tab->web_view, 
            webkit_web_view_get_zoom_level(tab->web_view) + 0.1);
    }
}

static void on_zoom_out_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        webkit_web_view_set_zoom_level(tab->web_view, 
            webkit_web_view_get_zoom_level(tab->web_view) - 0.1);
    }
}

static void on_zoom_reset_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        webkit_web_view_set_zoom_level(tab->web_view, 1.0);
    }
}

static void on_settings_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    // Simple settings dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Settings",
                                                     GTK_WINDOW(browser->window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Close", GTK_RESPONSE_CLOSE,
                                                     NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);
    
    // Home page setting
    GtkWidget *home_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), home_hbox, FALSE, FALSE, 0);
    
    GtkWidget *home_label = gtk_label_new("Home Page:");
    gtk_box_pack_start(GTK_BOX(home_hbox), home_label, FALSE, FALSE, 0);
    
    GtkWidget *home_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(home_entry), "about:blank");
    gtk_box_pack_start(GTK_BOX(home_hbox), home_entry, TRUE, TRUE, 0);
    
    // Search engine setting
    GtkWidget *search_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), search_hbox, FALSE, FALSE, 0);
    
    GtkWidget *search_label = gtk_label_new("Search Engine:");
    gtk_box_pack_start(GTK_BOX(search_hbox), search_label, FALSE, FALSE, 0);
    
    GtkWidget *search_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Bing");
    gtk_combo_box_set_active(GTK_COMBO_BOX(search_combo), 0);
    gtk_box_pack_start(GTK_BOX(search_hbox), search_combo, TRUE, TRUE, 0);
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_report_broken_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        const char *uri = webkit_web_view_get_uri(tab->web_view);
        if (uri && *uri) {
            char *report_url = g_strdup_printf("https://www.google.com/search?q=report+broken+site+%s", uri);
            webkit_web_view_load_uri(tab->web_view, report_url);
            g_free(report_url);
        }
    }
}

// Create the application menu
GtkWidget* create_application_menu(BrowserWindow *browser)
{
    GtkWidget *menu = gtk_menu_new();
    
    // File section
    GtkWidget *new_window_item = gtk_menu_item_new_with_label("New window");
    g_signal_connect(new_window_item, "activate", G_CALLBACK(on_new_window_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), new_window_item);
    
    GtkWidget *new_private_item = gtk_menu_item_new_with_label("New private window");
    g_signal_connect(new_private_item, "activate", G_CALLBACK(on_new_private_window_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), new_private_item);
    
    GtkWidget *sep1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep1);
    
    GtkWidget *bookmarks_item = gtk_menu_item_new_with_label("Bookmarks");
    g_signal_connect(bookmarks_item, "activate", G_CALLBACK(on_bookmarks_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), bookmarks_item);
    
    GtkWidget *history_item = gtk_menu_item_new_with_label("History");
    g_signal_connect(history_item, "activate", G_CALLBACK(on_history_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), history_item);
    
    GtkWidget *downloads_item = gtk_menu_item_new_with_label("Downloads");
    g_signal_connect(downloads_item, "activate", G_CALLBACK(on_downloads_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), downloads_item);
    
    GtkWidget *passwords_item = gtk_menu_item_new_with_label("Passwords");
    g_signal_connect(passwords_item, "activate", G_CALLBACK(on_passwords_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), passwords_item);
    
    GtkWidget *extensions_item = gtk_menu_item_new_with_label("Extensions and themes");
    g_signal_connect(extensions_item, "activate", G_CALLBACK(on_extensions_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), extensions_item);
    
    GtkWidget *sep2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep2);
    
    GtkWidget *print_item = gtk_menu_item_new_with_label("Print...");
    g_signal_connect(print_item, "activate", G_CALLBACK(on_print_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), print_item);
    
    GtkWidget *find_item = gtk_menu_item_new_with_label("Find in page...");
    g_signal_connect(find_item, "activate", G_CALLBACK(on_find_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), find_item);
    
    GtkWidget *sep3 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep3);
    
    // Zoom submenu
    GtkWidget *zoom_menu = gtk_menu_new();
    GtkWidget *zoom_item = gtk_menu_item_new_with_label("Zoom");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(zoom_item), zoom_menu);
    
    GtkWidget *zoom_in_item = gtk_menu_item_new_with_label("Zoom In");
    g_signal_connect(zoom_in_item, "activate", G_CALLBACK(on_zoom_in_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(zoom_menu), zoom_in_item);
    
    GtkWidget *zoom_out_item = gtk_menu_item_new_with_label("Zoom Out");
    g_signal_connect(zoom_out_item, "activate", G_CALLBACK(on_zoom_out_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(zoom_menu), zoom_out_item);
    
    GtkWidget *zoom_reset_item = gtk_menu_item_new_with_label("Reset Zoom");
    g_signal_connect(zoom_reset_item, "activate", G_CALLBACK(on_zoom_reset_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(zoom_menu), zoom_reset_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), zoom_item);
    
    GtkWidget *settings_item = gtk_menu_item_new_with_label("Settings");
    g_signal_connect(settings_item, "activate", G_CALLBACK(on_settings_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);
    
    GtkWidget *sep4 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep4);
    
    GtkWidget *report_item = gtk_menu_item_new_with_label("Report broken site");
    g_signal_connect(report_item, "activate", G_CALLBACK(on_report_broken_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), report_item);
    
    gtk_widget_show_all(menu);
    
    return menu;
}

void show_app_menu(GtkWidget *menu, GtkWidget *button)
{
    gtk_menu_popup_at_widget(GTK_MENU(menu), button,
                             GDK_GRAVITY_SOUTH_WEST,
                             GDK_GRAVITY_NORTH_WEST,
                             NULL);
}