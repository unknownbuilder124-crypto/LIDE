#include "wallpaper_settings.h"
#include <math.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <dirent.h>
#include <unistd.h>

/**
 * Structure to hold wallpaper data
 */
typedef struct {
    char *filename;
    char *display_name;
    char *full_path;
} Wallpaper;

/**
 * Comparison function for sorting wallpapers alphabetically
 */
static gint compare_wallpapers(gconstpointer a, gconstpointer b)
{
    Wallpaper *wa = (Wallpaper*)a;
    Wallpaper *wb = (Wallpaper*)b;
    return strcmp(wa->filename, wb->filename);
}

/**
 * Gets list of available wallpapers from ./images/wallpapers/
 * Looks for files matching wal*.png (e.g., wal1.png, wal2.png, etc.)
 *
 * @return GList of Wallpaper structures, sorted alphabetically
 */
static GList* get_available_wallpapers(void)
{
    GList *wallpapers = NULL;
    DIR *dir;
    struct dirent *entry;
    
    // Construct absolute path to wallpapers directory
    char wallpaper_dir[512];
    const char *home = g_getenv("HOME");
    if (!home) home = "/root";
    snprintf(wallpaper_dir, sizeof(wallpaper_dir), "%s/Desktop/LIDE/images/walpapers", home);
    
    // Try to open the wallpapers directory
    dir = opendir(wallpaper_dir);
    if (!dir) {
        fprintf(stderr, "Failed to open wallpapers directory: %s\n", wallpaper_dir);
        return NULL;
    }
    
    // Scan directory for wal*.png files
    while ((entry = readdir(dir)) != NULL) {
        // Check if file matches pattern wal*.png
        if (strncmp(entry->d_name, "wal", 3) == 0 && 
            strlen(entry->d_name) > 4 &&
            strcmp(entry->d_name + strlen(entry->d_name) - 4, ".png") == 0) {
            
            Wallpaper *wp = malloc(sizeof(Wallpaper));
            wp->filename = g_strdup(entry->d_name);
            
            // Create display name (e.g., "Wallpaper 1")
            char name_num[16];
            sscanf(entry->d_name, "wal%15[0-9]", name_num);
            wp->display_name = g_strdup_printf("Wallpaper %s", name_num);
            
            // Create full path
            wp->full_path = g_strdup_printf("%s/%s", wallpaper_dir, entry->d_name);
            
            wallpapers = g_list_append(wallpapers, wp);
        }
    }
    closedir(dir);
    
    // Sort wallpapers by filename
    wallpapers = g_list_sort(wallpapers, compare_wallpapers);
    
    return wallpapers;
}

/**
 * Applies wallpaper using feh command
 * Also sets a fallback solid color background
 *
 * @param wallpaper_path Full path to wallpaper image
 * @return TRUE if wallpaper was successfully applied, FALSE otherwise
 */
static gboolean apply_wallpaper(const char *wallpaper_path)
{
    FILE *log = fopen("/tmp/settings_apply_wallpaper.log", "a");
    if (!log) log = stderr;
    
    fprintf(log, "\n[%s] apply_wallpaper() called with: %s\n", __func__, wallpaper_path);
    fflush(log);
    
    // Only write the wallpaper path to the config file; do not set wallpaper here
    const char *home = g_getenv("HOME");
    if (!home) home = "/root";
    
    fprintf(log, "[%s] HOME=%s\n", __func__, home);
    fflush(log);
    
    char config_dir[512];
    snprintf(config_dir, sizeof(config_dir), "%s/.config/blackline", home);
    fprintf(log, "[%s] Config dir: %s\n", __func__, config_dir);
    fflush(log);
    
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p '%s'", config_dir);
    int mkdir_ret = system(mkdir_cmd);
    fprintf(log, "[%s] mkdir returned: %d\n", __func__, mkdir_ret);
    fflush(log);
    
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/current_wallpaper.conf", config_dir);
    fprintf(log, "[%s] Writing to: %s\n", __func__, config_path);
    fprintf(log, "[%s] Wallpaper path: %s\n", __func__, wallpaper_path);
    fflush(log);
    
    FILE *f = fopen(config_path, "w");
    if (f) {
        fprintf(f, "%s\n", wallpaper_path);
        fflush(f);
        fclose(f);
        fprintf(log, "[%s] Config file written successfully!\n", __func__);
        fflush(log);
        if (log != stderr) fclose(log);
        return TRUE;
    } else {
        fprintf(log, "[%s] ERROR: Failed to open config file for writing\n", __func__);
        fflush(log);
        if (log != stderr) fclose(log);
        return FALSE;
    }
}

/**
 * Callback for wallpaper combo box change
 * Applied when user selects a different wallpaper
 */
static void on_wallpaper_changed(GtkComboBox *combo, gpointer user_data)
{
    gchar *active_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!active_text) return;
    
    // Find selected wallpaper in list
    GList *wallpapers = (GList*)g_object_get_data(G_OBJECT(combo), "wallpapers");
    GList *iter;
    
        for (iter = wallpapers; iter; iter = iter->next) {
        Wallpaper *wp = (Wallpaper*)iter->data;
        if (strcmp(wp->display_name, active_text) == 0) {
            if (apply_wallpaper(wp->full_path)) {
                // Show success notification
                char success_msg[256];
                snprintf(success_msg, sizeof(success_msg), 
                         "Successfully applied: %s", wp->display_name);
                GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                           GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                           "Wallpaper Applied");
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                                         "%s", success_msg);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            } else {
                // Show error notification
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                         "Could not apply wallpaper: %s", wp->display_name);
                GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                           "Failed to Apply Wallpaper");
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                                         "%s", error_msg);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            }
            break;
        }
    }
    
    g_free(active_text);
}

/**
 * Frees a single Wallpaper structure.
 */
static void free_wallpaper_item(gpointer data)
{
    Wallpaper *wp = (Wallpaper*)data;
    if (!wp) return;
    g_free(wp->filename);
    g_free(wp->display_name);
    g_free(wp->full_path);
    free(wp);
}

/**
 * Frees a GList of Wallpaper items and the list itself.
 */
static void free_wallpapers_list(gpointer data)
{
    GList *list = (GList*)data;
    if (!list) return;
    g_list_free_full(list, free_wallpaper_item);
}

/**
 * Creates the wallpaper selection widget.
 * Scans for available wallpapers and creates a combo box interface.
 *
 * @return GtkWidget containing horizontally packed label and combo box
 */

// Handler for clicking a wallpaper thumbnail
static gint on_wallpaper_thumbnail_clicked(GtkWidget *event_box, GdkEventButton *event, gpointer user_data) {
    FILE *log = fopen("/tmp/settings_callback.log", "a");
    fprintf(log, "\n[CALLBACK] Triggered! user_data=%p\n", user_data);
    fflush(log);
    
    if (!user_data) {
        fprintf(log, "[CALLBACK] ERROR: No wallpaper data!\n");
        fflush(log);
        fclose(log);
        return FALSE;
    }
    
    Wallpaper *wp = (Wallpaper*)user_data;
    fprintf(log, "[CALLBACK] Selected: %s\n", wp->full_path ? wp->full_path : "NULL");
    fflush(log);
    
    if (!wp->full_path || !wp->filename) {
        fprintf(log, "[CALLBACK] ERROR: Invalid wallpaper struct!\n");
        fflush(log);
        fclose(log);
        fprintf(stderr, "[CALLBACK] Invalid wallpaper struct!\n");
        return FALSE;
    }
    
    fprintf(log, "[CALLBACK] Calling apply_wallpaper(%s)\n", wp->full_path);
    fflush(log);
    fclose(log);
    
    printf("[CALLBACK] Selected wallpaper: %s\n", wp->full_path);
    fflush(stdout);
    
    gboolean ok = apply_wallpaper(wp->full_path);
    if (ok) {
        char success_msg[256];
        snprintf(success_msg, sizeof(success_msg), "Successfully applied: %s", wp->filename);
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                  "Wallpaper Applied");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", success_msg);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Could not apply wallpaper: %s", wp->filename);
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Failed to Apply Wallpaper");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", error_msg);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    
    // finished
    return TRUE;
}

GtkWidget *wallpaper_settings_widget_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget *label = gtk_label_new("Select Wallpaper:");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // Get available wallpapers
    GList *wallpapers = get_available_wallpapers();
    if (!wallpapers) {
        const char *home = g_getenv("HOME");
        if (!home) home = "/root";
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "No wallpapers found in %s/Desktop/LIDE/images/walpapers/", home);
        GtkWidget *err_label = gtk_label_new(err_msg);
        gtk_box_pack_start(GTK_BOX(vbox), err_label, FALSE, FALSE, 0);
        return vbox;
    }

    // Create a flowbox for thumbnails
    GtkWidget *flowbox = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 4);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);

    GList *iter;
    for (iter = wallpapers; iter; iter = iter->next) {
        Wallpaper *wp = (Wallpaper*)iter->data;
        GtkWidget *thumb_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        // Load image as pixbuf and scale to fixed size (96x54)
        GError *error = NULL;
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(wp->full_path, &error);
        if (!pixbuf) {
            fprintf(stderr, "[DEBUG] Failed to load image: %s (%s)\n", wp->full_path, error ? error->message : "unknown error");
            GtkWidget *img_label = gtk_label_new("(Image error)");
            gtk_box_pack_start(GTK_BOX(thumb_box), img_label, FALSE, FALSE, 0);
            if (error) g_error_free(error);
        } else {
            int thumb_w = 96, thumb_h = 54;
            int img_w = gdk_pixbuf_get_width(pixbuf);
            int img_h = gdk_pixbuf_get_height(pixbuf);
            double scale = fmin((double)thumb_w/img_w, (double)thumb_h/img_h);
            int new_w = (int)(img_w * scale);
            int new_h = (int)(img_h * scale);
            GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, new_w, new_h, GDK_INTERP_BILINEAR);
            if (!scaled) {
                fprintf(stderr, "[DEBUG] Failed to scale image: %s\n", wp->full_path);
                GtkWidget *img_label = gtk_label_new("(Scale error)");
                gtk_box_pack_start(GTK_BOX(thumb_box), img_label, FALSE, FALSE, 0);
            } else {
                GtkWidget *image = gtk_image_new_from_pixbuf(scaled);
                gtk_box_pack_start(GTK_BOX(thumb_box), image, FALSE, FALSE, 0);
                g_object_unref(scaled);
            }
            g_object_unref(pixbuf);
        }
        GtkWidget *img_label = gtk_label_new(wp->filename);
        gtk_box_pack_start(GTK_BOX(thumb_box), img_label, FALSE, FALSE, 0);

        // Add a frame for OS-like look
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(frame), thumb_box);

        // Make clickable
        GtkWidget *event_box = gtk_event_box_new();
        gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), TRUE);
        gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
        gtk_container_add(GTK_CONTAINER(event_box), frame);
        g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_wallpaper_thumbnail_clicked), wp);

        gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), event_box, -1);
    }

    gtk_box_pack_start(GTK_BOX(vbox), flowbox, TRUE, TRUE, 0);

    // Attach the wallpapers list to the flowbox so it is freed when the
    // widget is destroyed. This prevents dangling pointers and crashes.
    g_object_set_data_full(G_OBJECT(flowbox), "wallpapers", wallpapers, free_wallpapers_list);

    return vbox;
}
