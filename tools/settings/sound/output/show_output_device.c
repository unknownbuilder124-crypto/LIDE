#include "output.h"
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <gtk/gtk.h>


/*
 * show_output_device.c
 * 
 * Audio output device list and selection
 * Enumerates playback devices (speakers, HDMI, USB). Sets default output.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

static GtkWidget *device_combo = NULL;
static pa_context *context = NULL;

static void sink_info_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    (void)userdata;
    if (eol < 0) return;
    if (!i) return;

    if (device_combo) {
        const char *desc = i->description ? i->description : i->name;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(device_combo), desc);
    }
}

static void default_sink_callback(pa_context *c, const pa_server_info *i, void *userdata) {
    (void)userdata;
    (void)c;
    if (!i) return;
}

static void on_device_changed(GtkComboBox *combo, gpointer data) {
    (void)data;
    gchar *selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!selected) return;
    /* Set default sink */
    if (context && pa_context_get_state(context) == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_set_default_sink(context, selected, NULL, NULL);
        if (op) pa_operation_unref(op);
    }
    g_free(selected);
}

static void init_pulse(void) {
    if (context) return;
    pa_glib_mainloop *mainloop = pa_glib_mainloop_new(g_main_context_default());
    pa_mainloop_api *api = pa_glib_mainloop_get_api(mainloop);
    context = pa_context_new(api, "BlackLine Settings");
    pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

/**
 * Creates the output device selection widget.
 *
 * @return GtkWidget containing the output device combo box.
 */
GtkWidget *output_device_selector_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Output Device</b>");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    device_combo = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(vbox), device_combo, FALSE, FALSE, 5);
    g_signal_connect(device_combo, "changed", G_CALLBACK(on_device_changed), NULL);

    init_pulse();

    /* Request sink list */
    if (context && pa_context_get_state(context) == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_get_sink_info_list(context, sink_info_callback, NULL);
        if (op) pa_operation_unref(op);
        op = pa_context_get_server_info(context, default_sink_callback, NULL);
        if (op) pa_operation_unref(op);
    }

    return vbox;
}