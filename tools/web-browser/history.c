#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HISTORY_FILE "history.txt"

static GList *history = NULL;
static BrowserWindow *global_browser = NULL;

/* Forward declarations for static functions */
static void on_history_item_clicked(GtkListBox *listbox, GtkListBoxRow *row, BrowserWindow *browser);
//gboolean on_close_tab_clicked(GtkButton *button, BrowserWindow *browser);

/**
 * Adds a URL to the browsing history.
 * Prevents duplicate consecutive entries and maintains a maximum of 100 entries.
 *
 * @param url   URL to add to history.
 * @param title Page title (if available).
 *
 * @sideeffect Updates history list and saves to disk.
 */
void add_to_history(const char *url, const char *title)
{
    printf("History: adding %s (%s)\n", url, title ? title : "no title");
    
    if (!url || !*url) return;
    
    /* Don't add about:blank to history */
    if (strcmp(url, "about:blank") == 0) return;
    
    /* Don't add empty URLs */
    if (strlen(url) < 4) return;
    
    /* Check if this URL is the same as the most recent entry (avoid duplicates) */
    if (history) {
        HistoryEntry *last = (HistoryEntry *)history->data;
        if (last && last->url && strcmp(last->url, url) == 0) {
            /* Update timestamp of existing entry instead of adding duplicate */
            last->timestamp = time(NULL);
            if (title && *title) {
                g_free(last->title);
                last->title = g_strdup(title);
            }
            save_history();
            printf("History: updated existing entry\n");
            return;
        }
    }
    
    HistoryEntry *entry = g_new(HistoryEntry, 1);
    entry->title = g_strdup(title && *title ? title : url);
    entry->url = g_strdup(url);
    entry->timestamp = time(NULL);
    
    history = g_list_prepend(history, entry);
    
    /* Keep history size manageable (last 100 entries) */
    if (g_list_length(history) > 100) {
        GList *last = g_list_last(history);
        HistoryEntry *old = (HistoryEntry *)last->data;
        g_free(old->title);
        g_free(old->url);
        g_free(old);
        history = g_list_delete_link(history, last);
    }
    
    save_history();
    printf("History: saved, now %d entries\n", g_list_length(history));
}

/**
 * Callback for history list item activation.
 * Loads the URL associated with the clicked history entry.
 *
 * @param listbox The GtkListBox containing history entries.
 * @param row     The activated row.
 * @param browser BrowserWindow instance.
 */
static void on_history_item_clicked(GtkListBox *listbox, GtkListBoxRow *row, BrowserWindow *browser)
{
    (void)listbox;
    const char *url = g_object_get_data(G_OBJECT(row), "url");
    if (url) {
        load_url(browser, url);
    }
}

/**
 * Clears all browsing history.
 *
 * @param button  The button that was clicked (unused).
 * @param browser BrowserWindow instance for UI updates.
 *
 * @sideeffect Frees all history entries and saves empty history.
 * @sideeffect Shows confirmation dialog.
 */
void clear_history(GtkButton *button, BrowserWindow *browser)
{
    (void)button;
    (void)browser;
    
    for (GList *l = history; l; l = l->next) {
        HistoryEntry *entry = (HistoryEntry *)l->data;
        g_free(entry->title);
        g_free(entry->url);
        g_free(entry);
    }
    g_list_free(history);
    history = NULL;
    
    save_history();
    
    /* Show confirmation */
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "History cleared successfully!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    /* Refresh the history tab if it's open */
    update_history_tab(browser);
}


/**
 * Updates or refreshes the history tab to reflect current history.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect If history tab is open, removes and recreates it with updated data.
 */
void update_history_tab(BrowserWindow *browser)
{
    /* Find and refresh the history tab if it's open */
    GtkWidget *notebook = browser->notebook;
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
        GtkWidget *tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), page);
        
        if (tab_label && GTK_IS_BOX(tab_label)) {
            GList *children = gtk_container_get_children(GTK_CONTAINER(tab_label));
            for (GList *c = children; c; c = c->next) {
                if (GTK_IS_LABEL(c->data)) {
                    const char *text = gtk_label_get_text(GTK_LABEL(c->data));
                    if (text && strcmp(text, "History") == 0) {
                        /* Found the history tab - remove and recreate it */
                        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), i);
                        g_list_free(children);
                        show_history_tab(browser);
                        return;
                    }
                }
            }
            g_list_free(children);
        }
    }
}

/**
 * Creates and displays the history tab.
 * Shows all visited pages in reverse chronological order with timestamps.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with history list.
 */
void show_history_tab(BrowserWindow *browser)
{
    global_browser = browser;
    
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);
    
    /* Header with title and clear button */
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(tab_content), header_box, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>History</span>");
    gtk_box_pack_start(GTK_BOX(header_box), title, FALSE, FALSE, 0);
    
    /* Clear history button */
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear History");
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(clear_history), browser);
    gtk_box_pack_end(GTK_BOX(header_box), clear_btn, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    if (!history) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new("No history yet");
        gtk_widget_set_margin_top(label, 50);
        gtk_widget_set_margin_bottom(label, 50);
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    } else {
        /* Show history in reverse chronological order */
        for (GList *l = history; l; l = l->next) {
            HistoryEntry *entry = (HistoryEntry *)l->data;
            
            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
            gtk_container_add(GTK_CONTAINER(row), vbox);
            gtk_widget_set_margin_top(row, 5);
            gtk_widget_set_margin_bottom(row, 5);
            gtk_widget_set_margin_start(row, 5);
            gtk_widget_set_margin_end(row, 5);
            
            GtkWidget *title_label = gtk_label_new(NULL);
            char *title_markup = g_strdup_printf("<span weight='bold'>%s</span>", entry->title);
            gtk_label_set_markup(GTK_LABEL(title_label), title_markup);
            g_free(title_markup);
            gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
            gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);
            
            GtkWidget *url_label = gtk_label_new(entry->url);
            gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
            gtk_widget_set_opacity(url_label, 0.7);
            gtk_box_pack_start(GTK_BOX(vbox), url_label, FALSE, FALSE, 0);
            
            char time_str[64];
            struct tm *tm_info = localtime(&entry->timestamp);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            
            GtkWidget *time_label = gtk_label_new(time_str);
            gtk_label_set_xalign(GTK_LABEL(time_label), 0.0);
            gtk_widget_set_opacity(time_label, 0.5);
            gtk_box_pack_start(GTK_BOX(vbox), time_label, FALSE, FALSE, 0);
            
            g_object_set_data_full(G_OBJECT(row), "url", g_strdup(entry->url), g_free);
            
            gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
        }
    }
    
    g_signal_connect(listbox, "row-activated", G_CALLBACK(on_history_item_clicked), browser);
    
    gtk_widget_show_all(tab_content);
    
    /* Create tab label with close button */
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

/**
 * Saves browsing history to disk.
 * Format: timestamp|title|url per line.
 *
 * @sideeffect Writes to HISTORY_FILE.
 */
void save_history(void)
{
    FILE *f = fopen(HISTORY_FILE, "w");
    if (!f) {
        printf("History: failed to open %s for writing\n", HISTORY_FILE);
        return;
    }
    
    for (GList *l = history; l; l = l->next) {
        HistoryEntry *entry = (HistoryEntry *)l->data;
        if (entry && entry->url) {
            fprintf(f, "%ld|%s|%s\n", entry->timestamp, entry->title, entry->url);
        }
    }
    
    fclose(f);
    printf("History: saved to %s\n", HISTORY_FILE);
}

/**
 * Loads browsing history from disk.
 * Reads HISTORY_FILE and populates the history list.
 *
 * @sideeffect Appends loaded history entries to global history list.
 * @note Call during application initialization to restore previous browsing history.
 */
void load_history(void)
{
    FILE *f = fopen(HISTORY_FILE, "r");
    if (!f) {
        printf("History: no history file found\n");
        return;
    }
    
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
    printf("History: loaded %d entries\n", g_list_length(history));
}