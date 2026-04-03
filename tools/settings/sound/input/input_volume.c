#include "input.h"
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <gtk/gtk.h>

/*
 * input_volume.c
 * 
 * Input audio volume level control
 * Adjusts mic input gain via ALSA/PulseAudio control API.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

static GtkWidget *input_scale = NULL;
static pa_context *context = NULL;

/**
 * Callback function for PulseAudio source information.
 *
 * Called when source volume information is available from the PulseAudio server.
 * Extracts the average volume from all channels and updates the UI scale widget.
 *
 * @param c        PulseAudio context pointer.
 * @param i        Source information structure containing volume data.
 * @param eol      End-of-list flag (non-zero when no more entries).
 * @param userdata User data pointer (unused, set to NULL).
 *
 * @sideeffect Updates input_scale widget value with current volume percentage.
 * @note Only updates if the source port is available (not disconnected).
 */
static void source_info_callback(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
    (void)userdata;
    if (eol < 0) return;
    if (!i) return;

    /* Update UI only if scale widget exists and source port is available */
    if (input_scale && GTK_IS_RANGE(input_scale) && i->active_port && i->active_port->available != PA_PORT_AVAILABLE_NO) {
        /* Convert PulseAudio volume (0..PA_VOLUME_NORM) to percentage (0..100) */
        double vol = (double)pa_cvolume_avg(&i->volume) / PA_VOLUME_NORM;
        gtk_range_set_value(GTK_RANGE(input_scale), vol * 100.0);
    }
}

/**
 * Sets input (microphone) volume through PulseAudio.
 *
 * @param percent Volume percentage (0-100).
 *
 * @sideeffect Sends set-source-volume command to PulseAudio server.
 * @note The operation is asynchronous; no return value is checked.
 */
static void set_input_volume(int percent) {
    if (!context || pa_context_get_state(context) != PA_CONTEXT_READY) return;

    pa_cvolume cv;
    /* Initialize volume structure for single channel (mono mic input) */
    pa_cvolume_set(&cv, 1, (pa_volume_t)(percent / 100.0 * PA_VOLUME_NORM));
    pa_operation *op = pa_context_set_source_volume_by_name(context, NULL, &cv, NULL, NULL);
    if (op) pa_operation_unref(op);
}

/**
 * GTK callback for input volume scale value changes.
 *
 * Triggered when user drags the volume slider.
 * Converts scale value to percentage and applies to system.
 *
 * @param range GTK range widget that emitted the signal.
 * @param data  User data (unused).
 *
 * @sideeffect Updates system input volume via set_input_volume().
 */
static void on_input_volume_changed(GtkRange *range, gpointer data) {
    (void)data;
    int val = (int)gtk_range_get_value(range);
    set_input_volume(val);
}

/**
 * Initializes PulseAudio context and connects to server.
 *
 * Creates a GLib mainloop integration for PulseAudio,
 * establishes context, and connects to the default PulseAudio server.
 * The context remains connected for the lifetime of the widget.
 *
 * @sideeffect Sets global context pointer.
 * @note Idempotent: does nothing if context already exists.
 * @warning Caller must ensure mainloop integration persists.
 */
static void init_pulse(void) {
    if (context) return;
    
    /* Create PulseAudio mainloop integrated with GLib's default context */
    pa_glib_mainloop *mainloop = pa_glib_mainloop_new(g_main_context_default());
    pa_mainloop_api *api = pa_glib_mainloop_get_api(mainloop);
    context = pa_context_new(api, "BlackLine Settings");
    pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

/**
 * Creates the input volume control widget.
 *
 * Constructs a vertical box containing a labeled scale widget for
 * microphone input volume control. Initializes PulseAudio connection
 * and queries current volume state.
 *
 * @return GtkWidget containing the complete input volume UI.
 *
 * @sideeffect Initializes PulseAudio context and sets up async volume query.
 * @note The returned widget owns the scale reference; caller manages lifecycle.
 */
GtkWidget *input_volume_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* Section header with markup formatting */
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Input Volume (Microphone)</b>");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    /* Volume slider: 0-100 range, 1-step increments */
    input_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(input_scale), TRUE);
    g_signal_connect(input_scale, "value-changed", G_CALLBACK(on_input_volume_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), input_scale, FALSE, FALSE, 5);

    /* Establish PulseAudio connection for volume control */
    init_pulse();

    /* Request current source volume if context is ready */
    if (context && pa_context_get_state(context) == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_get_source_info_by_name(context, NULL, source_info_callback, NULL);
        if (op) pa_operation_unref(op);
    }

    return vbox;
}