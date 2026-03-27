#include "resolution.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/**
 * Structure to hold resolution data
 */
typedef struct {
    int width;
    int height;
    char *mode_name;
    char *output;
} Resolution;

/**
 * Comparison function for sorting resolutions
 */
static gint compare_resolutions(gconstpointer a, gconstpointer b)
{
    Resolution *ra = (Resolution*)a;
    Resolution *rb = (Resolution*)b;
    if (ra->width != rb->width) return rb->width - ra->width;
    return rb->height - ra->height;
}

/**
 * Gets current display output and resolution
 */
static void get_current_display_info(char **output, int *width, int *height)
{
    FILE *fp = popen("xrandr --current | grep ' connected' | head -1", "r");
    if (!fp) return;
    
    char buffer[512];
    if (fgets(buffer, sizeof(buffer), fp)) {
        // Get output name
        char *output_name = strtok(buffer, " ");
        if (output_name && output_name[0]) {
            *output = g_strdup(output_name);
        }
        
        // Get current resolution with '*'
        char *res = strstr(buffer, "*");
        if (res) {
            char *start = res - 10;
            while (start > buffer && *start != ' ') start--;
            if (*start == ' ') start++;
            char *end = strchr(start, ' ');
            if (end) *end = '\0';
            sscanf(start, "%dx%d", width, height);
        }
    }
    pclose(fp);
}

/**
 * Applies new resolution using xrandr
 */
static gboolean apply_resolution(const char *output, int width, int height)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "xrandr --output %s --mode %dx%d", output, width, height);
    int result = system(cmd);
    
    if (result == 0) {
        // Refresh display as primary
        char primary_cmd[512];
        snprintf(primary_cmd, sizeof(primary_cmd), "xrandr --output %s --primary", output);
        system(primary_cmd);
        return TRUE;
    }
    return FALSE;
}

/**
 * Gets list of available resolutions for current display
 */
static GList* get_available_resolutions(const char *output)
{
    GList *resolutions = NULL;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "xrandr --current | grep -A 50 '%s' | grep -E '^\\s*[0-9]+x[0-9]+' | awk '{print $1}' | sort -u", output);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) > 0) {
            int w, h;
            sscanf(buffer, "%dx%d", &w, &h);
            
            // Check for duplicates
            gboolean exists = FALSE;
            GList *iter;
            for (iter = resolutions; iter; iter = iter->next) {
                Resolution *res = (Resolution*)iter->data;
                if (res->width == w && res->height == h) {
                    exists = TRUE;
                    break;
                }
            }
            
            if (!exists && w > 0 && h > 0) {
                Resolution *res = malloc(sizeof(Resolution));
                res->width = w;
                res->height = h;
                res->mode_name = g_strdup(buffer);
                res->output = g_strdup(output);
                resolutions = g_list_append(resolutions, res);
            }
        }
    }
    pclose(fp);
    
    // Sort resolutions
    resolutions = g_list_sort(resolutions, compare_resolutions);
    
    return resolutions;
}

/**
 * Format resolution string with aspect ratio
 */
static char* format_resolution(int width, int height)
{
    double ratio = (double)width / height;
    char *ratio_str;
    
    if (ratio > 1.77 && ratio < 1.78) ratio_str = " (16:9)";
    else if (ratio > 1.6 && ratio < 1.61) ratio_str = " (16:10)";
    else if (ratio > 1.33 && ratio < 1.34) ratio_str = " (4:3)";
    else if (ratio > 1.25 && ratio < 1.26) ratio_str = " (5:4)";
    else ratio_str = "";
    
    return g_strdup_printf("%d × %d%s", width, height, ratio_str);
}

/**
 * Callback for resolution combo box change
 */
static void on_resolution_changed(GtkComboBox *combo, gpointer user_data)
{
    gchar *active_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!active_text) return;
    
    int width, height;
    if (sscanf(active_text, "%d × %d", &width, &height) == 2) {
        // Find the resolution in our list
        GList *resolutions = (GList*)g_object_get_data(G_OBJECT(combo), "resolutions");
        GList *iter;
        for (iter = resolutions; iter; iter = iter->next) {
            Resolution *res = (Resolution*)iter->data;
            if (res->width == width && res->height == height) {
                if (apply_resolution(res->output, res->width, res->height)) {
                    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                               "Resolution changed to %dx%d\nClick OK to keep or wait 15 seconds to revert", 
                                                               width, height);
                    gtk_dialog_run(GTK_DIALOG(dialog));
                    gtk_widget_destroy(dialog);
                } else {
                    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                               "Failed to change resolution");
                    gtk_dialog_run(GTK_DIALOG(dialog));
                    gtk_widget_destroy(dialog);
                }
                break;
            }
        }
    }
    
    g_free(active_text);
}

/**
 * Creates the resolution selection widget.
 * Detects available resolutions and shows current resolution.
 *
 * @return GtkWidget containing vertically packed resolution options
 */
GtkWidget *resolution_widget_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    // Get current display info
    char *output = NULL;
    int current_w = 1920, current_h = 1080;
    get_current_display_info(&output, &current_w, &current_h);
    
    if (!output) {
        output = g_strdup("eDP-1"); // Fallback for laptop display
    }
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Resolution</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    // Current resolution info
    char *current_str = format_resolution(current_w, current_h);
    GtkWidget *current_label = gtk_label_new(NULL);
    char *current_markup = g_strdup_printf("<b>Current Resolution:</b>  %s", current_str);
    gtk_label_set_markup(GTK_LABEL(current_label), current_markup);
    gtk_label_set_xalign(GTK_LABEL(current_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), current_label, FALSE, FALSE, 5);
    g_free(current_str);
    g_free(current_markup);
    
    // Resolution selector
    GtkWidget *selector_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *label = gtk_label_new("Select Resolution:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    GtkWidget *combo = gtk_combo_box_text_new();
    
    // Get available resolutions
    GList *resolutions = get_available_resolutions(output);
    int active_index = -1;
    int index = 0;
    
    GList *iter;
    for (iter = resolutions; iter; iter = iter->next) {
        Resolution *res = (Resolution*)iter->data;
        char *display_str = format_resolution(res->width, res->height);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), display_str);
        g_free(display_str);
        
        if (res->width == current_w && res->height == current_h) {
            active_index = index;
        }
        index++;
    }
    
    // Store resolutions in combo box for callback
    g_object_set_data_full(G_OBJECT(combo), "resolutions", resolutions, 
                           (GDestroyNotify)g_list_free_full);
    
    // Set current resolution as active
    if (active_index >= 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active_index);
    } else if (resolutions) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    }
    
    // Connect signal
    g_signal_connect(combo, "changed", G_CALLBACK(on_resolution_changed), NULL);
    
    gtk_box_pack_start(GTK_BOX(selector_hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(selector_hbox), combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), selector_hbox, FALSE, FALSE, 5);
    
    // Apply button
    GtkWidget *apply_btn = gtk_button_new_with_label("Apply");
    gtk_widget_set_size_request(apply_btn, 80, 30);
    g_signal_connect_swapped(apply_btn, "clicked", G_CALLBACK(on_resolution_changed), combo);
    gtk_box_pack_start(GTK_BOX(vbox), apply_btn, FALSE, FALSE, 10);
    
    // Add spacing
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), spacer, TRUE, TRUE, 0);
    
    g_free(output);
    
    return vbox;
}