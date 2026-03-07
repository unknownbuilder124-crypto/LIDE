#include "bookmarks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOKMARKS_FILE "bookmarks.txt"

static GList *bookmarks = NULL;

// Bookmark menu item callback
static void on_bookmark_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    const char *url = g_object_get_data(G_OBJECT(item), "bookmark-url");
    if (url) {
        load_url(browser, url);
    }
}

// Add current page to bookmarks
static void on_add_bookmark_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (!tab || !tab->web_view) return;
    
    const char *url = webkit_web_view_get_uri(tab->web_view);
    const char *title = webkit_web_view_get_title(tab->web_view);
    
    if (url && *url) {
        // Create dialog for bookmark name
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Bookmark",
                                                        GTK_WINDOW(browser->window),
                                                        GTK_DIALOG_MODAL,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        "_Add", GTK_RESPONSE_ACCEPT,
                                                        NULL);
        
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
        gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);
        
        GtkWidget *name_label = gtk_label_new("Name:");
        gtk_widget_set_halign(name_label, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
        
        GtkWidget *name_entry = gtk_entry_new();
        if (title && *title) {
            gtk_entry_set_text(GTK_ENTRY(name_entry), title);
        } else {
            gtk_entry_set_text(GTK_ENTRY(name_entry), url);
        }
        gtk_box_pack_start(GTK_BOX(vbox), name_entry, FALSE, FALSE, 0);
        
        GtkWidget *url_label = gtk_label_new("URL:");
        gtk_widget_set_halign(url_label, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(vbox), url_label, FALSE, FALSE, 0);
        
        GtkWidget *url_entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(url_entry), url);
        gtk_editable_set_editable(GTK_EDITABLE(url_entry), FALSE);
        gtk_box_pack_start(GTK_BOX(vbox), url_entry, FALSE, FALSE, 0);
        
        gtk_widget_show_all(dialog);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            const char *bookmark_title = gtk_entry_get_text(GTK_ENTRY(name_entry));
            add_bookmark(browser, url, bookmark_title);
            save_bookmarks();
        }
        
        gtk_widget_destroy(dialog);
    }
}

// Manage bookmarks
static void on_manage_bookmarks_clicked(GtkMenuItem *item, BrowserWindow *browser)
{
    (void)item;
    show_bookmarks_tab(browser);
}

// Close tab callback
static void on_close_tab_clicked(GtkButton *button, BrowserWindow *browser)
{
    GtkWidget *tab_child = g_object_get_data(G_OBJECT(button), "tab-child");
    if (tab_child) {
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(browser->notebook), tab_child);
        if (page_num != -1) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), page_num);
        }
    }
}

// Create bookmarks menu
GtkWidget* create_bookmarks_menu(BrowserWindow *browser)
{
    GtkWidget *menu = gtk_menu_new();
    
    GtkWidget *add_bookmark_item = gtk_menu_item_new_with_label("Add Bookmark...");
    g_signal_connect(add_bookmark_item, "activate", G_CALLBACK(on_add_bookmark_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_bookmark_item);
    
    GtkWidget *manage_item = gtk_menu_item_new_with_label("Manage Bookmarks...");
    g_signal_connect(manage_item, "activate", G_CALLBACK(on_manage_bookmarks_clicked), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), manage_item);
    
    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
    
    // Add saved bookmarks
    for (GList *l = bookmarks; l; l = l->next) {
        Bookmark *bm = l->data;
        
        GtkWidget *item = gtk_menu_item_new_with_label(bm->title);
        g_object_set_data_full(G_OBJECT(item), "bookmark-url", g_strdup(bm->url), g_free);
        g_signal_connect(item, "activate", G_CALLBACK(on_bookmark_clicked), browser);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    
    gtk_widget_show_all(menu);
    
    return menu;
}

// Show bookmarks tab
void show_bookmarks_tab(BrowserWindow *browser)
{
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Bookmarks</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), title, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    for (GList *l = bookmarks; l; l = l->next) {
        Bookmark *bm = l->data;
        
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_container_add(GTK_CONTAINER(row), hbox);
        
        GtkWidget *label = gtk_label_new(bm->title);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
        
        GtkWidget *url_label = gtk_label_new(bm->url);
        gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
        gtk_widget_set_opacity(url_label, 0.7);
        gtk_box_pack_start(GTK_BOX(hbox), url_label, FALSE, FALSE, 20);
        
        GtkWidget *open_btn = gtk_button_new_with_label("Open");
        g_object_set_data_full(G_OBJECT(open_btn), "url", g_strdup(bm->url), g_free);
        g_signal_connect_swapped(open_btn, "clicked", G_CALLBACK(load_url), browser);
        gtk_box_pack_start(GTK_BOX(hbox), open_btn, FALSE, FALSE, 0);
        
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    }
    
    gtk_widget_show_all(tab_content);
    
    // Create tab label with close button
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Bookmarks");
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(close_btn, "Close tab");
    
    gtk_box_pack_start(GTK_BOX(tab_box), tab_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_box);
    
    g_object_set_data_full(G_OBJECT(close_btn), "tab-child", tab_content, NULL);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_tab_clicked), browser);
    
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(browser->notebook), tab_content, tab_box);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), page_num);
}

void add_bookmark(BrowserWindow *browser, const char *url, const char *title)
{
    (void)browser;
    
    // Check if already exists
    for (GList *l = bookmarks; l; l = l->next) {
        Bookmark *bm = l->data;
        if (strcmp(bm->url, url) == 0) {
            return; // Already bookmarked
        }
    }
    
    Bookmark *bm = g_new(Bookmark, 1);
    bm->title = g_strdup(title);
    bm->url = g_strdup(url);
    
    bookmarks = g_list_append(bookmarks, bm);
}

void show_bookmarks_menu(GtkWidget *menu, GtkWidget *button)
{
    gtk_menu_popup_at_widget(GTK_MENU(menu), button,
                             GDK_GRAVITY_SOUTH_WEST,
                             GDK_GRAVITY_NORTH_WEST,
                             NULL);
}

void save_bookmarks(void)
{
    FILE *f = fopen(BOOKMARKS_FILE, "w");
    if (!f) return;
    
    for (GList *l = bookmarks; l; l = l->next) {
        Bookmark *bm = l->data;
        fprintf(f, "%s|%s\n", bm->title, bm->url);
    }
    
    fclose(f);
}

void load_bookmarks(void)
{
    FILE *f = fopen(BOOKMARKS_FILE, "r");
    if (!f) return;
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        char *sep = strchr(line, '|');
        if (sep) {
            *sep = '\0';
            char *title = line;
            char *url = sep + 1;
            
            Bookmark *bm = g_new(Bookmark, 1);
            bm->title = g_strdup(title);
            bm->url = g_strdup(url);
            bookmarks = g_list_append(bookmarks, bm);
        }
    }
    
    fclose(f);
}