#include "downloads.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DOWNLOADS_FILE "downloads.txt"
#define DOWNLOAD_DIR "Downloads"

static GList *downloads = NULL;

// Forward declarations
static void open_download_folder(GtkButton *button);
static void clear_completed_downloads(GtkButton *button, BrowserWindow *browser);
static void on_close_tab_clicked(GtkButton *button, BrowserWindow *browser);

// Create downloads directory if it doesn't exist
static void ensure_download_dir(void)
{
    struct stat st = {0};
    if (stat(DOWNLOAD_DIR, &st) == -1) {
        mkdir(DOWNLOAD_DIR, 0700);
    }
}

static void open_download_folder(GtkButton *button)
{
    const char *path = g_object_get_data(G_OBJECT(button), "path");
    if (path) {
        char *cmd = g_strdup_printf("xdg-open %s", DOWNLOAD_DIR);
        system(cmd);
        g_free(cmd);
    }
}

static void clear_completed_downloads(GtkButton *button, BrowserWindow *browser)
{
    (void)button;
    GList *new_list = NULL;
    
    for (GList *l = downloads; l; l = l->next) {
        DownloadItem *item = l->data;
        if (item->status != 2) { // Keep if not complete
            new_list = g_list_append(new_list, item);
        } else {
            g_free(item->filename);
            g_free(item->url);
            g_free(item->destination);
            g_free(item);
        }
    }
    
    g_list_free(downloads);
    downloads = new_list;
    
    save_downloads();
    // Refresh the current tab if it's the downloads tab
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "Completed downloads cleared. Close and reopen the Downloads tab to see changes.");
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

void show_downloads_tab(BrowserWindow *browser)
{
    ensure_download_dir();
    
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Downloads</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), title, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    for (GList *l = downloads; l; l = l->next) {
        DownloadItem *item = l->data;
        
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(row), vbox);
        
        GtkWidget *filename_label = gtk_label_new(item->filename);
        gtk_label_set_xalign(GTK_LABEL(filename_label), 0.0);
        gtk_widget_set_margin_top(filename_label, 5);
        gtk_box_pack_start(GTK_BOX(vbox), filename_label, FALSE, FALSE, 0);
        
        GtkWidget *url_label = gtk_label_new(item->url);
        gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
        gtk_widget_set_opacity(url_label, 0.7);
        gtk_box_pack_start(GTK_BOX(vbox), url_label, FALSE, FALSE, 0);
        
        char status_text[256];
        if (item->status == 0) snprintf(status_text, sizeof(status_text), "Pending");
        else if (item->status == 1) snprintf(status_text, sizeof(status_text), "Downloading... %.1f%%", item->progress);
        else if (item->status == 2) snprintf(status_text, sizeof(status_text), "Complete");
        else snprintf(status_text, sizeof(status_text), "Failed");
        
        GtkWidget *status_label = gtk_label_new(status_text);
        gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
        
        if (item->status == 2) {
            GtkWidget *open_btn = gtk_button_new_with_label("Open Folder");
            char *path = g_build_filename(DOWNLOAD_DIR, item->filename, NULL);
            g_object_set_data_full(G_OBJECT(open_btn), "path", g_strdup(path), g_free);
            g_signal_connect(open_btn, "clicked", G_CALLBACK(open_download_folder), NULL);
            gtk_box_pack_start(GTK_BOX(vbox), open_btn, FALSE, FALSE, 0);
            g_free(path);
        }
        
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    }
    
    // Clear completed button
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear Completed");
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(clear_completed_downloads), browser);
    gtk_box_pack_start(GTK_BOX(tab_content), clear_btn, FALSE, FALSE, 0);
    
    gtk_widget_show_all(tab_content);
    
    // Create tab label with close button
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Downloads");
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

void add_download(const char *url, const char *filename)
{
    ensure_download_dir();
    
    DownloadItem *item = g_new(DownloadItem, 1);
    item->url = g_strdup(url);
    item->filename = g_strdup(filename);
    item->destination = g_build_filename(DOWNLOAD_DIR, filename, NULL);
    item->progress = 0;
    item->status = 0; // Pending
    
    downloads = g_list_append(downloads, item);
    
    // Simulate download (in real browser, would use WebKit download)
    item->status = 2; // Mark as complete for demo
    item->progress = 100;
    
    save_downloads();
}

void save_downloads(void)
{
    FILE *f = fopen(DOWNLOADS_FILE, "w");
    if (!f) return;
    
    for (GList *l = downloads; l; l = l->next) {
        DownloadItem *item = l->data;
        fprintf(f, "%s|%s|%s|%f|%d\n", 
                item->filename, item->url, item->destination, 
                item->progress, item->status);
    }
    
    fclose(f);
}

void load_downloads(void)
{
    FILE *f = fopen(DOWNLOADS_FILE, "r");
    if (!f) return;
    
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        DownloadItem *item = g_new(DownloadItem, 1);
        
        char *p = line;
        char *sep = strchr(p, '|');
        if (!sep) { g_free(item); continue; }
        *sep = '\0';
        item->filename = g_strdup(p);
        
        p = sep + 1;
        sep = strchr(p, '|');
        if (!sep) { g_free(item->filename); g_free(item); continue; }
        *sep = '\0';
        item->url = g_strdup(p);
        
        p = sep + 1;
        sep = strchr(p, '|');
        if (!sep) { g_free(item->filename); g_free(item->url); g_free(item); continue; }
        *sep = '\0';
        item->destination = g_strdup(p);
        
        p = sep + 1;
        sep = strchr(p, '|');
        if (!sep) { g_free(item->filename); g_free(item->url); g_free(item->destination); g_free(item); continue; }
        *sep = '\0';
        item->progress = atof(p);
        
        p = sep + 1;
        item->status = atoi(p);
        
        downloads = g_list_append(downloads, item);
    }
    
    fclose(f);
}