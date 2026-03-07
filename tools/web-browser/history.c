#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HISTORY_FILE "history.txt"

static GList *history = NULL;

// Forward declarations for static functions
static void on_history_item_clicked(GtkListBox *listbox, GtkListBoxRow *row, BrowserWindow *browser);
static void clear_history(GtkButton *button, BrowserWindow *browser);
static void on_close_tab_clicked(GtkButton *button, BrowserWindow *browser);

void add_to_history(const char *url, const char *title)
{
    if (!url || !*url) return;
    
    // Don't add about:blank to history
    if (strcmp(url, "about:blank") == 0) return;
    
    HistoryEntry *entry = g_new(HistoryEntry, 1);
    entry->title = g_strdup(title && *title ? title : url);
    entry->url = g_strdup(url);
    entry->timestamp = time(NULL);
    
    history = g_list_prepend(history, entry);
    
    // Keep history size manageable (last 100 entries)
    if (g_list_length(history) > 100) {
        GList *last = g_list_last(history);
        HistoryEntry *old = last->data;
        g_free(old->title);
        g_free(old->url);
        g_free(old);
        history = g_list_delete_link(history, last);
    }
    
    save_history();
}

static void on_history_item_clicked(GtkListBox *listbox, GtkListBoxRow *row, BrowserWindow *browser)
{
    (void)listbox;
    const char *url = g_object_get_data(G_OBJECT(row), "url");
    if (url) {
        load_url(browser, url);
    }
}

static void clear_history(GtkButton *button, BrowserWindow *browser)
{
    (void)button;
    (void)browser;
    
    for (GList *l = history; l; l = l->next) {
        HistoryEntry *entry = l->data;
        g_free(entry->title);
        g_free(entry->url);
        g_free(entry);
    }
    g_list_free(history);
    history = NULL;
    
    save_history();
    // Refresh the current tab if it's the history tab, but we'll just recreate it
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "History cleared. Close and reopen the History tab to see changes.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

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

void show_history_tab(BrowserWindow *browser)
{
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>History</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), title, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    // Show history in reverse chronological order
    for (GList *l = history; l; l = l->next) {
        HistoryEntry *entry = l->data;
        
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_container_add(GTK_CONTAINER(row), vbox);
        
        GtkWidget *title_label = gtk_label_new(entry->title);
        gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
        gtk_widget_set_margin_top(title_label, 5);
        gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);
        
        GtkWidget *url_label = gtk_label_new(entry->url);
        gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
        gtk_widget_set_opacity(url_label, 0.7);
        gtk_box_pack_start(GTK_BOX(vbox), url_label, FALSE, FALSE, 0);
        
        char time_str[64];
        struct tm *tm_info = localtime(&entry->timestamp);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
        
        GtkWidget *time_label = gtk_label_new(time_str);
        gtk_label_set_xalign(GTK_LABEL(time_label), 0.0);
        gtk_widget_set_opacity(time_label, 0.5);
        gtk_box_pack_start(GTK_BOX(vbox), time_label, FALSE, FALSE, 0);
        
        g_object_set_data_full(G_OBJECT(row), "url", g_strdup(entry->url), g_free);
        
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    }
    
    g_signal_connect(listbox, "row-activated", G_CALLBACK(on_history_item_clicked), browser);
    
    // Clear history button
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear History");
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(clear_history), browser);
    gtk_box_pack_start(GTK_BOX(tab_content), clear_btn, FALSE, FALSE, 0);
    
    gtk_widget_show_all(tab_content);
    
    // Create tab label with close button
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("History");
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

void save_history(void)
{
    FILE *f = fopen(HISTORY_FILE, "w");
    if (!f) return;
    
    for (GList *l = history; l; l = l->next) {
        HistoryEntry *entry = l->data;
        fprintf(f, "%ld|%s|%s\n", entry->timestamp, entry->title, entry->url);
    }
    
    fclose(f);
}

void load_history(void)
{
    FILE *f = fopen(HISTORY_FILE, "r");
    if (!f) return;
    
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        char *first_sep = strchr(line, '|');
        if (!first_sep) continue;
        *first_sep = '\0';
        
        char *second_sep = strchr(first_sep + 1, '|');
        if (!second_sep) continue;
        *second_sep = '\0';
        
        time_t timestamp = atol(line);
        char *title = first_sep + 1;
        char *url = second_sep + 1;
        
        HistoryEntry *entry = g_new(HistoryEntry, 1);
        entry->timestamp = timestamp;
        entry->title = g_strdup(title);
        entry->url = g_strdup(url);
        
        history = g_list_append(history, entry);
    }
    
    fclose(f);
}