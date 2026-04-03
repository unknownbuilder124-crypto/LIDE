#include "output.h"
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <gtk/gtk.h>

/*
 * output_volume.c
 * 
 * Output audio volume level control
 * Adjusts speaker/headphone output volume via PulseAudio.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

static pa_glib_mainloop *mainloop = NULL;
static pa_context *context = NULL;
static GtkWidget *volume_scale = NULL;
static GtkWidget *balance_scale = NULL;

/**
 * PulseAudio context state change callback.
 *
 * Called when the connection state to PulseAudio server changes.
 * Triggers initial volume query once the context becomes ready.
 *
 * @param c        PulseAudio context pointer.
 * @param userdata User data (unused, set to NULL).
 *
 * @sideeffect Initiates sink info request when context reaches READY state.
 */
static void context_state_callback(pa_context *c, void *userdata) {
    (void)userdata;
    pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_READY) {
        /* Context connected - request initial sink volume information */
        pa_operation *op = pa_context_get_sink_info_by_name(c, NULL, NULL, NULL);
        if (op) pa_operation_unref(op);
    }
}

/**
 * PulseAudio sink information callback.
 *
 * Called when sink volume and channel information is received.
 * Updates both volume and balance UI widgets based on current sink state.
 *
 * @param c        PulseAudio context pointer.
 * @param i        Sink information structure containing volume and channel data.
 * @param eol      End-of-list flag (non-zero when no more entries).
 * @param userdata User data (unused, set to NULL).
 *
 * @sideeffect Updates volume_scale and balance_scale widget values.
 * @note Balance calculation: (right - left) / (left + right) * 100
 *       Negative values indicate left bias, positive indicates right bias.
 */
static void sink_info_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    (void)userdata;
    if (eol < 0) return;
    if (!i) return;

    /* Only update if sink port is available (not disconnected) */
    if (i && i->active_port && i->active_port->available != PA_PORT_AVAILABLE_NO) {
        /* Update master volume display */
        if (volume_scale && GTK_IS_RANGE(volume_scale)) {
            double vol = (double)pa_cvolume_avg(&i->volume) / PA_VOLUME_NORM;
            gtk_range_set_value(GTK_RANGE(volume_scale), vol * 100.0);
        }
        
        /* Update balance control for stereo output (at least 2 channels) */
        if (balance_scale && GTK_IS_RANGE(balance_scale) && i->volume.channels >= 2) {
            double left = (double)i->volume.values[0] / PA_VOLUME_NORM;
            double right = (double)i->volume.values[1] / PA_VOLUME_NORM;
            
            /* Calculate balance: 0 = center, negative = left bias, positive = right bias */
            double balance = (right - left) / (left + right) * 100.0;
            if (left + right == 0) balance = 0;  /* Avoid division by zero */
            
            gtk_range_set_value(GTK_RANGE(balance_scale), balance);
        }
    }
}

/**
 * Sets master output volume through PulseAudio.
 *
 * @param percent Volume percentage (0-100).
 *
 * @sideeffect Sends set-sink-volume command to PulseAudio server.
 * @note Assumes stereo output (2 channels) for volume setting.
 */
static void set_volume(int percent) {
    if (!context || pa_context_get_state(context) != PA_CONTEXT_READY) return;

    pa_cvolume cv;
    /* Set volume for both stereo channels to same value */
    pa_cvolume_set(&cv, 2, (pa_volume_t)(percent / 100.0 * PA_VOLUME_NORM));
    pa_operation *op = pa_context_set_sink_volume_by_name(context, NULL, &cv, NULL, NULL);
    if (op) pa_operation_unref(op);
}

/**
 * Sets audio balance between left and right channels.
 *
 * Balance algorithm preserves total perceived volume by adjusting
 * left and right channels proportionally while maintaining their sum.
 *
 * @param balance Balance value (-100 to +100):
 *                -100 = full left channel only
 *                    0 = equal volume both channels
 *               +100 = full right channel only
 *
 * @sideeffect Sends per-channel volume adjustment to PulseAudio.
 * @warning Current implementation does not preserve existing volume level.
 *          A production implementation should query current volume first.
 */
static void set_balance(double balance) {
    if (!context || pa_context_get_state(context) != PA_CONTEXT_READY) return;

    /* Note: Production code should query current volume before adjusting balance.
     * This implementation assumes nominal volume (PA_VOLUME_NORM) as baseline. */
    pa_cvolume cv;
    pa_cvolume_set(&cv, 2, PA_VOLUME_NORM);
    
    /* Balance mapping: left channel = (100 - balance)/200, right = (100 + balance)/200
     * This maintains total power: left^2 + right^2 normalized when sum = 1.0 */
    double left = (100.0 - balance) / 200.0;
    double right = (100.0 + balance) / 200.0;
    
    /* Clamp to valid range [0, 1] */
    if (left < 0) left = 0;
    if (right < 0) right = 0;
    if (left > 1) left = 1;
    if (right > 1) right = 1;
    
    cv.values[0] = left * PA_VOLUME_NORM;
    cv.values[1] = right * PA_VOLUME_NORM;
    
    pa_operation *op = pa_context_set_sink_volume_by_name(context, NULL, &cv, NULL, NULL);
    if (op) pa_operation_unref(op);
}

/**
 * GTK callback for master volume scale changes.
 *
 * Triggered when user drags the volume slider.
 *
 * @param range GTK range widget that emitted the signal.
 * @param data  User data (unused).
 *
 * @sideeffect Updates system output volume via set_volume().
 */
static void on_volume_changed(GtkRange *range, gpointer data) {
    (void)data;
    int val = (int)gtk_range_get_value(range);
    set_volume(val);
}

/**
 * GTK callback for balance scale changes.
 *
 * Triggered when user drags the balance slider.
 *
 * @param range GTK range widget that emitted the signal.
 * @param data  User data (unused).
 *
 * @sideeffect Updates channel balance via set_balance().
 */
static void on_balance_changed(GtkRange *range, gpointer data) {
    (void)data;
    double val = gtk_range_get_value(range);
    set_balance(val);
}

/**
 * Initializes PulseAudio context with GLib mainloop integration.
 *
 * Creates a mainloop tied to the default GLib context,
 * sets up state callback for connection monitoring,
 * and initiates connection to PulseAudio server.
 *
 * @sideeffect Sets global mainloop and context pointers.
 * @note Idempotent: does nothing if context already exists.
 * @warning Context remains connected until application exit.
 */
static void init_pulse(void) {
    if (context) return;

    /* Create PulseAudio mainloop that uses GLib's event loop */
    mainloop = pa_glib_mainloop_new(g_main_context_default());
    pa_mainloop_api *api = pa_glib_mainloop_get_api(mainloop);
    context = pa_context_new(api, "BlackLine Settings");
    pa_context_set_state_callback(context, context_state_callback, NULL);
    pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

/**
 * Creates the output volume and balance control widget.
 *
 * Constructs a vertical box containing labeled scale widgets for
 * master volume and stereo balance. Initializes PulseAudio connection
 * for volume control.
 *
 * @return GtkWidget containing the complete output volume UI.
 *
 * @sideeffect Initializes PulseAudio context.
 * @note Volume range: 0-100%, Balance range: -100 to +100.
 */
GtkWidget *output_volume_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* Master volume section */
    GtkWidget *vol_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(vol_label), "<b>Master Volume</b>");
    gtk_box_pack_start(GTK_BOX(vbox), vol_label, FALSE, FALSE, 0);

    volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(volume_scale), TRUE);
    g_signal_connect(volume_scale, "value-changed", G_CALLBACK(on_volume_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), volume_scale, FALSE, FALSE, 5);

    /* Stereo balance section */
    GtkWidget *bal_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(bal_label), "<b>Balance</b> (Left ← → Right)");
    gtk_box_pack_start(GTK_BOX(vbox), bal_label, FALSE, FALSE, 0);

    balance_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -100, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(balance_scale), TRUE);
    g_signal_connect(balance_scale, "value-changed", G_CALLBACK(on_balance_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), balance_scale, FALSE, FALSE, 5);

    /* Initialize PulseAudio subsystem */
    init_pulse();

    return vbox;
}

/**
 * Refreshes output volume display from current system state.
 *
 * Public API function to manually trigger volume/balance UI update.
 * Queries PulseAudio for current sink information and updates widgets
 * via sink_info_callback.
 *
 * @sideeffect Asynchronously requests sink info from PulseAudio server.
 * @note Safe to call multiple times; operation is non-blocking.
 */
void output_volume_refresh(void) {
    if (context && pa_context_get_state(context) == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_get_sink_info_by_name(context, NULL, sink_info_callback, NULL);
        if (op) pa_operation_unref(op);
    }
}