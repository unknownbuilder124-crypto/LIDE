#include "bluetooth.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Function: create_bluetooth_tab
 * -------------------------------
 * Constructs the Bluetooth settings tab UI displaying command reference
 * from blt.txt. Text content is read-only and cannot be edited by user.
 * 
 * Returns: GtkWidget* - Fully constructed GtkBox containing scrolled
 *          text view with Bluetooth command documentation.
 * 
 * Side effects: Allocates GTK widgets, attempts to read blt.txt from
 *               multiple locations, modifies text buffer.
 */
GtkWidget* create_bluetooth_tab(void) {
    // Main container for the tab
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    // Scrolled window for the text content
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    
    // Text view widget to display the blt.txt content
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    
    // Set monospace font for better readability of commands
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
    gtk_widget_override_font(text_view, font_desc);
    pango_font_description_free(font_desc);
    
    gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    
    // Read blt.txt file and populate text buffer
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);
    
    FILE *fp = NULL;
    char line[1024];
    
    // Try multiple possible locations for blt.txt
    if (access("tools/settings/bluetooth/blt.txt", F_OK) == 0) {
        fp = fopen("tools/settings/bluetooth/blt.txt", "r");
    } else if (access("blt.txt", F_OK) == 0) {
        fp = fopen("blt.txt", "r");
    } else if (access("/usr/local/share/blackline-bluetooth-commands.txt", F_OK) == 0) {
        fp = fopen("/usr/local/share/blackline-bluetooth-commands.txt", "r");
    } else {
        // Try using popen with cat command
        fp = popen("cat tools/settings/bluetooth/blt.txt 2>/dev/null", "r");
        if (!fp) {
            fp = popen("cat blt.txt 2>/dev/null", "r");
        }
    }
    
    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp) != NULL) {
            gtk_text_buffer_insert(buffer, &iter, line, -1);
        }
        pclose(fp);
    } else {
        // Fallback message if file cannot be found
        gtk_text_buffer_insert(buffer, &iter, 
            "Bluetooth Command Reference\n"
            "===========================\n\n"
            "Error: Could not locate blt.txt\n\n"
            "Expected locations:\n"
            "  - tools/settings/bluetooth/blt.txt\n"
            "  - ./blt.txt\n"
            "  - /usr/local/share/blackline-bluetooth-commands.txt\n\n"
            "Basic Bluetooth commands:\n"
            "  sudo systemctl status bluetooth\n"
            "  sudo systemctl start bluetooth\n"
            "  bluetoothctl power on\n"
            "  bluetoothctl scan on\n"
            "  bluetoothctl devices\n"
            "  rfkill list\n", -1);
    }
    
    return box;
}