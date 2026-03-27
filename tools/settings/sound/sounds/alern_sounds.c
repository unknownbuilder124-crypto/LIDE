#include "sound.h"
#include <gtk/gtk.h>

GtkWidget *alert_sounds_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Alert Sounds</b>");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // Simple switch to enable/disable system sounds
    GtkWidget *check = gtk_check_button_new_with_label("Enable system sounds");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, FALSE, 5);

    // A combobox for selecting the sound theme
    GtkWidget *theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Default");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Freedesktop");
    gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), theme_combo, FALSE, FALSE, 5);

    return vbox;
}