#include "app_menu.h"
#include "bookmarks.h"
#include "history.h"
#include "downloads.h"
#include "download-stats.h"
#include "passwords.h"
#include "extensions.h"
#include "settings.h"
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/* External references (declared in voidfox.h) */

/*
 * app_menu.c
 * 
 * Application menu construction and event dispatch
Builds hierarchical menu structure for File/Edit/View/Help. Routes
menu item selections to corresponding handlers.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

extern void show_bookmarks_tab(BrowserWindow *browser);
extern void show_history_tab(BrowserWindow *browser);
extern void show_downloads_tab(BrowserWindow *browser);
extern void show_passwords_tab(BrowserWindow *browser);
extern void show_extensions_tab(BrowserWindow *browser);
extern void show_themes_tab(BrowserWindow *browser);
extern void show_settings_tab(BrowserWindow *browser);

/* Global reference to downloads menu item for updating badge */
static GtkWidget *downloads_menu_item = NULL;

/**
 * Find in page dialog data structure.
 * Holds UI elements and state for the find-in-page dialog.
 */
typedef struct {
    GtkWidget *dialog;                     /* Find dialog widget */
    GtkWidget *entry;                      /* Search text entry */
    GtkWidget *case_sensitive_check;       /* Checkbox for case-sensitive search */
    GtkWidget *wrap_check;                 /* Checkbox for wrap-around search */
    WebKitWebView *web_view;               /* Web view to search within */
    WebKitFindController *find_controller; /* WebKit find controller */
} FindDialogData;

/* Find in page callbacks */

/**
 * Finds the next occurrence of the search text.
 *
 * @param button The button that was clicked (unused).
 * @param data   FindDialogData containing search parameters.
 */
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

/**
 * Finds the previous occurrence of the search text.
 *
 * @param button The button that was clicked (unused).
 * @param data   FindDialogData containing search parameters.
 */
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

/**
 * Callback for find dialog response (close button or escape key).
 * Stops highlighting and destroys the dialog.
 *
 * @param dialog      The find dialog.
 * @param response_id Response ID from dialog.
 * @param data        FindDialogData to clean up.
 */
static void find_dialog_response(GtkDialog *dialog, gint response_id, FindDialogData *data)

{
    if (response_id == GTK_RESPONSE_CLOSE || response_id == GTK_RESPONSE_DELETE_EVENT) {
        /* Stop highlighting when closing */
        webkit_find_controller_search_finish(data->find_controller);
        gtk_widget_destroy(GTK_WIDGET(dialog));
        g_free(data);
    }
}

/**
 * Callback for Enter key in find entry.
 * Triggers find next operation.
 *
 * @param entry The entry widget.
 * @param data  FindDialogData.
 */
static void find_entry_activate(GtkEntry *entry, FindDialogData *data)

{
    (void)entry;
    find_next(NULL, data);
}

/* Menu item callbacks */

/**
 * Callback for New Window menu item.
 * Launches a new browser instance.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance (unused).
 *
 * @sideeffect Forks a child process to launch new browser.
 */
static void on_new_window_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    /* Launch a new browser window */
    pid_t pid = fork();
    if (pid == 0) {
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    }
}

/**
 * Callback for New Private Window menu item.
 * Launches a new private browsing instance.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance (unused).
 *
 * @sideeffect Forks a child process to launch private browser.
 */
static void on_new_private_window_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    /* Launch a new private window */
    pid_t pid = fork();
    if (pid == 0) {
        execl("./voidfox", "voidfox", "--private", NULL);
        exit(0);
    }
}

/**
 * Callback for Bookmarks menu item.
 * Shows the bookmarks tab.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_bookmarks_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    show_bookmarks_tab(browser);
}

/**
 * Callback for History menu item.
 * Shows the history tab.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_history_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    show_history_tab(browser);
}

/**
 * Callback for Downloads menu item.
 * Shows the downloads tab.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_downloads_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    show_downloads_tab(browser);
}

/**
 * Updates the downloads menu item to show active download count.
 * Shows "Downloads (N)" where N is the number of active downloads.
 */
void update_downloads_menu_badge(void)
{
    if (!downloads_menu_item) return;
    
    int active_count = 0;
    for (GList *l = downloads; l; l = l->next) {
        DownloadItem *item = l->data;
        if (item->status == 1) { /* downloading */
            active_count++;
        }
    }
    
    char *label_text;
    if (active_count > 0) {
        label_text = g_strdup_printf("Downloads (%d)", active_count);
    } else {
        label_text = g_strdup("Downloads");
    }
    
    gtk_menu_item_set_label(GTK_MENU_ITEM(downloads_menu_item), label_text);
    g_free(label_text);
}

/**
 * Callback for Passwords menu item.
 * Shows the saved passwords tab.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_passwords_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    show_passwords_tab(browser);
}

/**
 * Callback for Themes menu item.
 * Shows the themes tab.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_themes_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    show_themes_tab(browser);
}

/**
 * Callback for Print menu item.
 * Triggers browser print dialog via JavaScript.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_print_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        /* Use non-deprecated function to execute JavaScript print */
        webkit_web_view_evaluate_javascript(tab->web_view, "window.print();", -1, NULL, NULL, NULL, NULL, NULL);
    }
}

/**
 * Callback for Find in Page menu item.
 * Opens the find dialog for the current tab.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_find_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (!tab || !tab->web_view) return;
    
    /* Create find dialog */
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
    
    /* Search entry */
    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    
    GtkWidget *search_label = gtk_label_new("Search:");
    gtk_box_pack_start(GTK_BOX(hbox1), search_label, FALSE, FALSE, 0);
    
    data->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(data->entry), "Enter text to find...");
    gtk_box_pack_start(GTK_BOX(hbox1), data->entry, TRUE, TRUE, 0);
    
    /* Options */
    GtkWidget *options_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), options_box, FALSE, FALSE, 0);
    
    data->case_sensitive_check = gtk_check_button_new_with_label("Case sensitive");
    gtk_box_pack_start(GTK_BOX(options_box), data->case_sensitive_check, FALSE, FALSE, 0);
    
    data->wrap_check = gtk_check_button_new_with_label("Wrap around");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->wrap_check), TRUE);
    gtk_box_pack_start(GTK_BOX(options_box), data->wrap_check, FALSE, FALSE, 0);
    
    /* Navigation buttons */
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    
    GtkWidget *prev_btn = gtk_button_new_with_label("Previous");
    GtkWidget *next_btn = gtk_button_new_with_label("Next");
    gtk_box_pack_start(GTK_BOX(hbox2), prev_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), next_btn, TRUE, TRUE, 0);
    
    /* Connect signals */
    g_signal_connect(next_btn, "clicked", G_CALLBACK(find_next), data);
    g_signal_connect(prev_btn, "clicked", G_CALLBACK(find_previous), data);
    g_signal_connect(data->entry, "activate", G_CALLBACK(find_entry_activate), data);
    
    g_signal_connect(data->dialog, "response", G_CALLBACK(find_dialog_response), data);
    
    gtk_widget_show_all(data->dialog);
}

/**
 * Callback for Zoom In menu item.
 * Increases zoom level of the current tab by 0.1.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
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

/**
 * Callback for Zoom Out menu item.
 * Decreases zoom level of the current tab by 0.1.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
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

/**
 * Callback for Reset Zoom menu item.
 * Resets zoom level to 1.0.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
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

/**
 * Callback for Settings menu item.
 * Shows the settings tab.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
static void on_settings_clicked(GtkMenuItem *item, BrowserWindow *browser)

{
    (void)item;
    show_settings_tab(browser);
}

/**
 * Callback for Report Broken Site menu item.
 * Opens a Google search with "report broken site" and current URL.
 *
 * @param item    The menu item.
 * @param browser BrowserWindow instance.
 */
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

/**
 * Creates the application menu with all menu items.
 *
 * @param browser BrowserWindow instance for callbacks.
 * @return GtkWidget containing the complete menu.
 */
GtkWidget* create_application_menu(BrowserWindow *browser)

{
    GtkWidget *menu = gtk_menu_new();
    
    /* File section */
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
    downloads_menu_item = downloads_item; /* Store global reference */
    g_signal_connect(downloads_item, "activate", G_CALLBACK(on_downloads_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), downloads_item);
    
    /* Update the badge initially */
    update_downloads_menu_badge();
    
    GtkWidget *passwords_item = gtk_menu_item_new_with_label("Passwords");
    g_signal_connect(passwords_item, "activate", G_CALLBACK(on_passwords_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), passwords_item);

    GtkWidget *themes_item = gtk_menu_item_new_with_label("Themes");
    g_signal_connect(themes_item, "activate", G_CALLBACK(on_themes_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), themes_item);
    
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
    
    /* Zoom submenu */
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
    
    /* Settings item */
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

/**
 * Displays the application menu anchored to a button.
 *
 * @param menu   The menu to display.
 * @param button The button to anchor the menu to.
 */
void show_app_menu(GtkWidget *menu, GtkWidget *button)

{
    gtk_menu_popup_at_widget(GTK_MENU(menu), button,GDK_GRAVITY_SOUTH_WEST,GDK_GRAVITY_NORTH_WEST, NULL);
}