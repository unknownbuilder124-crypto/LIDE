#include "downloads.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define DOWNLOADS_FILE "downloads.txt"
#define DOWNLOAD_DIR "Downloads"

GList *downloads = NULL;
BrowserWindow *global_browser = NULL;

/* Forward declarations for static functions */
//gboolean on_close_tab_clicked(GtkButton *button, BrowserWindow *browser);
static void open_download_folder(GtkButton *button, DownloadItem *item);
static void remove_download(DownloadItem *item);
static void clear_completed_downloads(GtkButton *button, BrowserWindow *browser);
static void on_download_finished(DownloadItem *item);
static void on_download_failed(DownloadItem *item);

/**
 * Creates the downloads directory if it does not exist.
 * Directory is created with 0700 permissions (owner read/write/execute only).
 *
 * @sideeffect Creates directory ./Downloads in the current working directory.
 */
static void ensure_download_dir(void)

{
    struct stat st = {0};
    if (stat(DOWNLOAD_DIR, &st) == -1) {
        mkdir(DOWNLOAD_DIR, 0700);
    }
}

/**
 * Extracts a filename from a URL.
 * Strips query parameters and returns the last path component.
 *
 * @param url The URL to extract filename from.
 * @return Newly allocated string containing the filename.
 *         Defaults to "download.bin" on failure.
 */
static char* get_filename_from_url(const char *url)

{
    if (!url) return g_strdup("download.bin");
    
    const char *last_slash = strrchr(url, '/');
    if (last_slash && *(last_slash + 1)) {
        /* Remove query parameters */
        char *filename = g_strdup(last_slash + 1);
        char *question = strchr(filename, '?');
        if (question) *question = '\0';
        return filename;
    }
    return g_strdup("download.bin");
}

/* Callbacks for download signals */

/**
 * Callback for download completion.
 * Updates the download item status to complete and saves to disk.
 *
 * @param item The DownloadItem that finished.
 *
 * @sideeffect Marks item as complete (status=2) with 100% progress.
 * @sideeffect Saves downloads to file and updates UI.
 */
static void on_download_finished(DownloadItem *item)

{
    item->status = 2; /* complete */
    item->progress = 100.0;
    save_downloads();
    
    if (global_browser) {
        update_downloads_tab(global_browser);
    }
}

/**
 * Callback for download failure.
 * Updates the download item status to failed and stores error message.
 *
 * @param item The DownloadItem that failed.
 *
 * @sideeffect Marks item as failed (status=3) with error message.
 * @sideeffect Saves downloads to file and updates UI.
 */
static void on_download_failed(DownloadItem *item)

{
    item->status = 3; /* failed */
    item->error_message = g_strdup("Download failed");
    save_downloads();
    
    if (global_browser) {
        update_downloads_tab(global_browser);
    }
}

/**
 * Adds a new download to the download manager.
 * Creates a DownloadItem, sets up destination, and connects signals.
 *
 * @param download The WebKitDownload object.
 * @param browser  BrowserWindow instance for UI updates.
 *
 * @sideeffect Creates a download directory if needed.
 * @sideeffect Appends to global downloads list and saves to file.
 */
void add_download(WebKitDownload *download, BrowserWindow *browser)

{
    ensure_download_dir();
    global_browser = browser;
    
    DownloadItem *item = g_new0(DownloadItem, 1);
    
    /* Get the URI from the download request */
    WebKitURIRequest *request = webkit_download_get_request(download);
    if (request) {
        const gchar *uri = webkit_uri_request_get_uri(request);
        if (uri) {
            item->url = g_strdup(uri);
            item->filename = get_filename_from_url(uri);
        } else {
            item->url = g_strdup("Unknown URL");
            item->filename = g_strdup("download.bin");
        }
    } else {
        item->url = g_strdup("Unknown URL");
        item->filename = g_strdup("download.bin");
    }
    
    item->progress = 0;
    item->status = 1; /* downloading (start immediately) */
    item->received = 0;
    item->total = 0;
    item->download = g_object_ref(download);
    
    /* Set destination path */
    char *dest_path = g_build_filename(DOWNLOAD_DIR, item->filename, NULL);
    item->destination = dest_path;
    webkit_download_set_destination(download, dest_path);
    webkit_download_set_allow_overwrite(download, FALSE);
    
    downloads = g_list_append(downloads, item);
    
    /* Connect simple signal handlers */
    g_signal_connect_swapped(download, "finished", G_CALLBACK(on_download_finished), item);
    g_signal_connect_swapped(download, "failed", G_CALLBACK(on_download_failed), item);
    
    save_downloads();
    
    /* Refresh the downloads tab if it's open */
    update_downloads_tab(browser);
}

/**
 * Opens the downloads folder in the system file manager.
 *
 * @param button The button that was clicked (unused).
 * @param item   The DownloadItem (unused, folder is global).
 *
 * @sideeffect Executes xdg-open to open the download directory.
 */
static void open_download_folder(GtkButton *button, DownloadItem *item)

{
    (void)button;
    (void)item;
    char *cmd = g_strdup_printf("xdg-open %s", DOWNLOAD_DIR);
    system(cmd);
    g_free(cmd);
}

/**
 * Removes a download from the manager.
 *
 * @param item The DownloadItem to remove.
 *
 * @sideeffect Removes item from global list and frees all associated memory.
 * @sideeffect Saves remaining downloads to file.
 */
static void remove_download(DownloadItem *item)

{
    downloads = g_list_remove(downloads, item);
    if (item->download) g_object_unref(item->download);
    g_free(item->filename);
    g_free(item->url);
    g_free(item->destination);
    g_free(item->error_message);
    g_free(item);
    save_downloads();
}


/**
 * Clears completed and failed downloads from the list.
 * Keeps only active (downloading) items.
 *
 * @param button  The button that was clicked.
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Removes non-active downloads and saves the list.
 */
static void clear_completed_downloads(GtkButton *button, BrowserWindow *browser)

{
    (void)button;
    GList *new_list = NULL;
    
    for (GList *l = downloads; l; l = l->next) {
        DownloadItem *item = l->data;
        if (item->status == 1) { /* Keep if downloading */
            new_list = g_list_append(new_list, item);
        } else {
            if (item->download) g_object_unref(item->download);
            g_free(item->filename);
            g_free(item->url);
            g_free(item->destination);
            g_free(item->error_message);
            g_free(item);
        }
    }
    
    g_list_free(downloads);
    downloads = new_list;
    
    save_downloads();
    update_downloads_tab(browser);
}

/**
 * Updates or creates the downloads tab to reflect current state.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Refreshes existing downloads tab or creates a new one.
 */
void update_downloads_tab(BrowserWindow *browser)

{
    /* Find and refresh the downloads tab if it's open */
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
                    if (text && strcmp(text, "Downloads") == 0) {
                        /* Found the downloads tab - remove and recreate it */
                        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), i);
                        g_list_free(children);
                        show_downloads_tab(browser);
                        return;
                    }
                }
            }
            g_list_free(children);
        }
    }
    
    /* If we get here, the downloads tab isn't open, so we'll create a new one */
    show_downloads_tab(browser);
}

/**
 * Creates and displays the downloads management tab.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with download list.
 */
void show_downloads_tab(BrowserWindow *browser)

{
    global_browser = browser;
    ensure_download_dir();
    
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);
    
    /* Header with title and clear button */
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(tab_content), header_box, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Downloads</span>");
    gtk_box_pack_start(GTK_BOX(header_box), title, FALSE, FALSE, 0);
    
    /* Clear completed button */
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear Completed");
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(clear_completed_downloads), browser);
    gtk_box_pack_end(GTK_BOX(header_box), clear_btn, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    if (!downloads) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new("No downloads yet");
        gtk_widget_set_margin_top(label, 50);
        gtk_widget_set_margin_bottom(label, 50);
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    } else {
        for (GList *l = downloads; l; l = l->next) {
            DownloadItem *item = l->data;
            
            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_container_add(GTK_CONTAINER(row), vbox);
            gtk_widget_set_margin_top(row, 5);
            gtk_widget_set_margin_bottom(row, 5);
            gtk_widget_set_margin_start(row, 5);
            gtk_widget_set_margin_end(row, 5);
            
            /* Filename and URL */
            GtkWidget *filename_label = gtk_label_new(NULL);
            char *filename_markup = g_strdup_printf("<span weight='bold'>%s</span>", item->filename);
            gtk_label_set_markup(GTK_LABEL(filename_label), filename_markup);
            g_free(filename_markup);
            gtk_label_set_xalign(GTK_LABEL(filename_label), 0.0);
            gtk_box_pack_start(GTK_BOX(vbox), filename_label, FALSE, FALSE, 0);
            
            GtkWidget *url_label = gtk_label_new(item->url);
            gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
            gtk_widget_set_opacity(url_label, 0.7);
            gtk_box_pack_start(GTK_BOX(vbox), url_label, FALSE, FALSE, 0);
            
            /* Status text */
            GtkWidget *status_label = gtk_label_new(NULL);
            char status_text[256];
            
            if (item->status == 0) {
                snprintf(status_text, sizeof(status_text), "Pending...");
            } else if (item->status == 1) {
                snprintf(status_text, sizeof(status_text), "Downloading...");
            } else if (item->status == 2) {
                snprintf(status_text, sizeof(status_text), "Complete");
            } else if (item->status == 3) {
                snprintf(status_text, sizeof(status_text), "Failed");
            }
            
            gtk_label_set_text(GTK_LABEL(status_label), status_text);
            gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
            gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
            
            /* Action buttons */
            GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
            
            if (item->status == 2) {
                /* Open folder button for completed downloads */
                GtkWidget *open_btn = gtk_button_new_with_label("Open Folder");
                g_signal_connect(open_btn, "clicked", G_CALLBACK(open_download_folder), item);
                gtk_box_pack_start(GTK_BOX(button_box), open_btn, FALSE, FALSE, 0);
            }
            
            /* Remove button for all items */
            GtkWidget *remove_btn = gtk_button_new_with_label("Remove");
            g_signal_connect_swapped(remove_btn, "clicked", G_CALLBACK(remove_download), item);
            gtk_box_pack_start(GTK_BOX(button_box), remove_btn, FALSE, FALSE, 0);
            
            gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
        }
    }
    
    gtk_widget_show_all(tab_content);
    
    /* Create tab label with close button */
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

/**
 * Saves all downloads to disk.
 * Format: filename|url|destination|status per line.
 *
 * @sideeffect Writes to DOWNLOADS_FILE.
 */
void save_downloads(void)
{
    FILE *f = fopen(DOWNLOADS_FILE, "w");
    if (!f) return;
    
    for (GList *l = downloads; l; l = l->next) {
        DownloadItem *item = l->data;
        fprintf(f, "%s|%s|%s|%d\n", 
                item->filename, item->url, 
                item->destination ? item->destination : "",
                item->status);
    }
    
    fclose(f);
}

/**
 * Loads downloads from disk.
 * Reads DOWNLOADS_FILE and populates the downloads list.
 *
 * @sideeffect Appends loaded downloads to global downloads list.
 * @note Call during application initialization to restore previous downloads.
 */
void load_downloads(void)
{
    FILE *f = fopen(DOWNLOADS_FILE, "r");
    if (!f) return;
    
    char line[8192];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        DownloadItem *item = g_new0(DownloadItem, 1);
        
        /* Parse: filename|url|destination|status */
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
        item->status = atoi(p);
        
        item->progress = (item->status == 2) ? 100.0 : 0.0;
        item->download = NULL;
        
        downloads = g_list_append(downloads, item);
    }
    
    fclose(f);
}