#include "output.h"
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <gtk/gtk.h>

static pa_glib_mainloop *mainloop = NULL;
static pa_context *context = NULL;
static GtkWidget *volume_scale = NULL;
static GtkWidget *balance_scale = NULL;

static void context_state_callback(pa_context *c, void *userdata) {
    (void)userdata;
    pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_READY) {
        // Request initial volume
        pa_operation *op = pa_context_get_sink_info_by_name(c, NULL, NULL, NULL);
        if (op) pa_operation_unref(op);
    }
}

static void sink_info_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    (void)userdata;
    if (eol < 0) return;
    if (!i) return;

    if (i && i->active_port && i->active_port->available != PA_PORT_AVAILABLE_NO) {
        if (volume_scale && GTK_IS_RANGE(volume_scale)) {
            double vol = (double)pa_cvolume_avg(&i->volume) / PA_VOLUME_NORM;
            gtk_range_set_value(GTK_RANGE(volume_scale), vol * 100.0);
        }
        if (balance_scale && GTK_IS_RANGE(balance_scale) && i->volume.channels >= 2) {
            double left = (double)i->volume.values[0] / PA_VOLUME_NORM;
            double right = (double)i->volume.values[1] / PA_VOLUME_NORM;
            double balance = (right - left) / (left + right) * 100.0;
            if (left + right == 0) balance = 0;
            gtk_range_set_value(GTK_RANGE(balance_scale), balance);
        }
    }
}

static void set_volume(int percent) {
    if (!context || pa_context_get_state(context) != PA_CONTEXT_READY) return;

    pa_cvolume cv;
    pa_cvolume_set(&cv, 2, (pa_volume_t)(percent / 100.0 * PA_VOLUME_NORM));
    pa_operation *op = pa_context_set_sink_volume_by_name(context, NULL, &cv, NULL, NULL);
    if (op) pa_operation_unref(op);
}

static void set_balance(double balance) {
    if (!context || pa_context_get_state(context) != PA_CONTEXT_READY) return;

    // Get current volume from sink
    pa_operation *op = pa_context_get_sink_info_by_name(context, NULL, NULL, NULL);
    // In a real implementation you'd need to store the current volume and adjust channels.
    // For simplicity, we'll just query and set both channels.
    // This is a placeholder; a proper implementation would retrieve the current volume.
    pa_cvolume cv;
    pa_cvolume_set(&cv, 2, PA_VOLUME_NORM);
    // Apply balance: left = (100 - balance)/2, right = (100 + balance)/2
    double left = (100.0 - balance) / 200.0;
    double right = (100.0 + balance) / 200.0;
    cv.values[0] = left * PA_VOLUME_NORM;
    cv.values[1] = right * PA_VOLUME_NORM;
    op = pa_context_set_sink_volume_by_name(context, NULL, &cv, NULL, NULL);
    if (op) pa_operation_unref(op);
}

static void on_volume_changed(GtkRange *range, gpointer data) {
    (void)data;
    int val = (int)gtk_range_get_value(range);
    set_volume(val);
}

static void on_balance_changed(GtkRange *range, gpointer data) {
    (void)data;
    double val = gtk_range_get_value(range);
    set_balance(val);
}

static void init_pulse(void) {
    if (context) return;

    mainloop = pa_glib_mainloop_new(g_main_context_default());
    pa_mainloop_api *api = pa_glib_mainloop_get_api(mainloop);
    context = pa_context_new(api, "BlackLine Settings");
    pa_context_set_state_callback(context, context_state_callback, NULL);
    pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

GtkWidget *output_volume_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    // Volume label and scale
    GtkWidget *vol_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(vol_label), "<b>Master Volume</b>");
    gtk_box_pack_start(GTK_BOX(vbox), vol_label, FALSE, FALSE, 0);

    volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(volume_scale), TRUE);
    g_signal_connect(volume_scale, "value-changed", G_CALLBACK(on_volume_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), volume_scale, FALSE, FALSE, 5);

    // Balance label and scale
    GtkWidget *bal_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(bal_label), "<b>Balance</b> (Left ← → Right)");
    gtk_box_pack_start(GTK_BOX(vbox), bal_label, FALSE, FALSE, 0);

    balance_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -100, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(balance_scale), TRUE);
    g_signal_connect(balance_scale, "value-changed", G_CALLBACK(on_balance_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), balance_scale, FALSE, FALSE, 5);

    init_pulse();

    return vbox;
}

void output_volume_refresh(void) {
    if (context && pa_context_get_state(context) == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_get_sink_info_by_name(context, NULL, sink_info_callback, NULL);
        if (op) pa_operation_unref(op);
    }
}