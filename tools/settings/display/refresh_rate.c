#include "refresh_rate.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/**
 * Structure to hold refresh rate data
 */
typedef struct {
    int rate;
    char *mode_name;
    char *output;
} RefreshRate;

/**
 * Comparison function for sorting refresh rates
 */
static gint compare_refresh_rates(gconstpointer a, gconstpointer b)
{
    int rate_a = GPOINTER_TO_INT(a);
    int rate_b = GPOINTER_TO_INT(b);
    return rate_b - rate_a;
}

/**
 * Gets current refresh rate using xrandr
 */
static int get_current_refresh_rate(void)
{
    FILE *fp = popen("xrandr --current | grep '*' | head -1 | grep -o '[0-9]*\\.[0-9]*' | head -1", "r");
    if (!fp) return 60;
    
    char buffer[256];
    int rate = 60;
    if (fgets(buffer, sizeof(buffer), fp)) {
        rate = (int)(atof(buffer) + 0.5);
    }
    pclose(fp);
    return rate;
}

/**
 * Gets current display output
 */
static char* get_current_output(void)
{
    FILE *fp = popen("xrandr --current | grep ' connected' | awk '{print $1}'", "r");
    char output[256];
    if (!fgets(output, sizeof(output), fp)) {
        pclose(fp);
        return g_strdup("eDP-1");
    }
    output[strcspn(output, "\n")] = 0;
    pclose(fp);
    return g_strdup(output);
}

/**
 * Gets available refresh rates for current resolution
 */
static GList* get_available_refresh_rates(const char *output)
{
    GList *rates = NULL;
    
    // Get current resolution
    FILE *fp = popen("xrandr --current | grep '*' | head -1 | grep -o '[0-9]*x[0-9]*'", "r");
    char resolution[256];
    if (!fgets(resolution, sizeof(resolution), fp)) {
        pclose(fp);
        return NULL;
    }
    resolution[strcspn(resolution, "\n")] = 0;
    pclose(fp);
    
    // Get modes for current output and resolution
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "xrandr --current | grep -A 100 '%s' | grep -E '^\\s*%s' | grep -o '[0-9]*\\.[0-9]*\\*' | tr -d '*\\n' | sort -u",
             output, resolution);
    fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) > 0) {
            int rate = (int)(atof(buffer) + 0.5);
            
            // Check if rate already exists
            gboolean exists = FALSE;
            GList *iter;
            for (iter = rates; iter; iter = iter->next) {
                int existing = GPOINTER_TO_INT(iter->data);
                if (existing == rate) {
                    exists = TRUE;
                    break;
                }
            }
            
            if (!exists && rate > 0) {
                rates = g_list_append(rates, GINT_TO_POINTER(rate));
            }
        }
    }
    pclose(fp);
    
    // Sort rates in descending order
    rates = g_list_sort(rates, compare_refresh_rates);
    
    // If no rates detected, provide defaults
    if (!rates) {
        int default_rates[] = {240, 144, 120, 75, 60, 50};
        for (int i = 0; i < 6; i++) {
            rates = g_list_append(rates, GINT_TO_POINTER(default_rates[i]));
        }
        rates = g_list_sort(rates, compare_refresh_rates);
    }
    
    return rates;
}

/**
 * Applies new refresh rate using xrandr
 */
static gboolean apply_refresh_rate(const char *output, int rate)
{
    // Get current resolution
    FILE *fp = popen("xrandr --current | grep '*' | head -1 | grep -o '[0-9]*x[0-9]*'", "r");
    char resolution[256];
    if (!fgets(resolution, sizeof(resolution), fp)) {
        pclose(fp);
        return FALSE;
    }
    resolution[strcspn(resolution, "\n")] = 0;
    pclose(fp);
    
    char apply_cmd[512];
    snprintf(apply_cmd, sizeof(apply_cmd), "xrandr --output %s --mode %s --rate %d", output, resolution, rate);
    return system(apply_cmd) == 0;
}

/**
 * Callback for refresh rate combo box change
 */
static void on_refresh_rate_changed(GtkComboBox *combo, gpointer user_data)
{
    gchar *active_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!active_text) return;
    
    int rate;
    if (sscanf(active_text, "%d Hz", &rate) == 1) {
        char *output = (char*)g_object_get_data(G_OBJECT(combo), "output");
        if (apply_refresh_rate(output, rate)) {
            GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                       "Refresh rate changed to %d Hz", rate);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        } else {
            GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                       "Failed to change refresh rate to %d Hz.\nMake sure your display supports this rate.", rate);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }
    
    g_free(active_text);
}

/**
 * Creates the refresh rate selection widget.
 * Detects available refresh rates and shows current rate.
 *
 * @return GtkWidget containing vertically packed refresh rate options
 */
GtkWidget *refresh_rate_widget_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    // Get current display output
    char *output = get_current_output();
    
    // Get current refresh rate
    int current_rate = get_current_refresh_rate();
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Refresh Rate</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    // Current refresh rate info
    char *current_markup = g_strdup_printf("<b>Current Refresh Rate:</b>  %d Hz", current_rate);
    GtkWidget *current_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(current_label), current_markup);
    gtk_label_set_xalign(GTK_LABEL(current_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), current_label, FALSE, FALSE, 5);
    g_free(current_markup);
    
    // Refresh rate selector
    GtkWidget *selector_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *label = gtk_label_new("Select Rate:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    GtkWidget *combo = gtk_combo_box_text_new();
    
    // Get available refresh rates
    GList *rates = get_available_refresh_rates(output);
    int active_index = -1;
    int index = 0;
    
    GList *iter;
    for (iter = rates; iter; iter = iter->next) {
        int rate = GPOINTER_TO_INT(iter->data);
        char display_str[32];
        snprintf(display_str, sizeof(display_str), "%d Hz", rate);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), display_str);
        
        if (rate == current_rate) {
            active_index = index;
        }
        index++;
    }
    
    // Store output in combo box for callback
    g_object_set_data_full(G_OBJECT(combo), "output", output, g_free);
    
    // Set current refresh rate as active
    if (active_index >= 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active_index);
    } else if (rates) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    }
    
    // Connect signal
    g_signal_connect(combo, "changed", G_CALLBACK(on_refresh_rate_changed), NULL);
    
    gtk_box_pack_start(GTK_BOX(selector_hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(selector_hbox), combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), selector_hbox, FALSE, FALSE, 5);
    
    // Apply button
    GtkWidget *apply_btn = gtk_button_new_with_label("Apply");
    gtk_widget_set_size_request(apply_btn, 80, 30);
    g_signal_connect_swapped(apply_btn, "clicked", G_CALLBACK(on_refresh_rate_changed), combo);
    gtk_box_pack_start(GTK_BOX(vbox), apply_btn, FALSE, FALSE, 10);
    
    // Free rates list
    g_list_free(rates);
    
    // Add spacing
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), spacer, TRUE, TRUE, 0);
    
    return vbox;
}