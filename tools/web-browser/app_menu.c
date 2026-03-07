#include "app_menu.h"
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    // Launch a new private window (could add --private flag later)
    pid_t pid = fork();
    if (pid == 0) {
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    }
}

static void on_print_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        webkit_web_view_run_javascript(tab->web_view, "window.print();", NULL, NULL, NULL);
    }
}

static void on_save_page_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        // Simple save dialog - could be enhanced later
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Page As",
                                                        GTK_WINDOW(browser->window),
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        "_Save", GTK_RESPONSE_ACCEPT,
                                                        NULL);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            // Could implement actual save functionality here
            g_free(filename);
        }
        gtk_widget_destroy(dialog);
    }
}

static void on_find_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    // Simple find dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Find in Page",
                                                     GTK_WINDOW(browser->window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Close", GTK_RESPONSE_CLOSE,
                                                     NULL);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Search...");
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 10);
    gtk_widget_show_all(dialog);
    
    // Could implement find functionality here
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_translate_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        const char *uri = webkit_web_view_get_uri(tab->web_view);
        if (uri && *uri) {
            char *translate_url = g_strdup_printf("https://translate.google.com/translate?sl=auto&tl=en&u=%s", uri);
            webkit_web_view_load_uri(tab->web_view, translate_url);
            g_free(translate_url);
        }
    }
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
    // Simple settings dialog - could be expanded
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "VoidFox Settings\n\n"
                                              "Home Page: about:blank\n"
                                              "Search Engine: Google\n"
                                              "Hardware Acceleration: Disabled");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_history_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    // Simple history view - could be expanded
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "History feature coming soon!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_downloads_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "Downloads feature coming soon!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_passwords_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "Password manager coming soon!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_extensions_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "Extensions support coming soon!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_report_broken_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "Report broken site feature coming soon!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
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
    
    GtkWidget *save_page_item = gtk_menu_item_new_with_label("Save page as...");
    g_signal_connect(save_page_item, "activate", G_CALLBACK(on_save_page_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), save_page_item);
    
    GtkWidget *find_item = gtk_menu_item_new_with_label("Find in page...");
    g_signal_connect(find_item, "activate", G_CALLBACK(on_find_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), find_item);
    
    GtkWidget *translate_item = gtk_menu_item_new_with_label("Translate page...");
    g_signal_connect(translate_item, "activate", G_CALLBACK(on_translate_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), translate_item);
    
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