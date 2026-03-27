#include "input.h"
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <gtk/gtk.h>

static GtkWidget *input_scale = NULL;
static pa_context *context = NULL;

static void source_info_callback(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
    (void)userdata;
    if (eol < 0) return;
    if (!i) return;

    if (input_scale && GTK_IS_RANGE(input_scale) && i->active_port && i->active_port->available != PA_PORT_AVAILABLE_NO) {
        double vol = (double)pa_cvolume_avg(&i->volume) / PA_VOLUME_NORM;
        gtk_range_set_value(GTK_RANGE(input_scale), vol * 100.0);
    }
}

static void set_input_volume(int percent) {
    if (!context || pa_context_get_state(context) != PA_CONTEXT_READY) return;

    pa_cvolume cv;
    pa_cvolume_set(&cv, 1, (pa_volume_t)(percent / 100.0 * PA_VOLUME_NORM));
    pa_operation *op = pa_context_set_source_volume_by_name(context, NULL, &cv, NULL, NULL);
    if (op) pa_operation_unref(op);
}

static void on_input_volume_changed(GtkRange *range, gpointer data) {
    (void)data;
    int val = (int)gtk_range_get_value(range);
    set_input_volume(val);
}

static void init_pulse(void) {
    if (context) return;
    pa_glib_mainloop *mainloop = pa_glib_mainloop_new(g_main_context_default());
    pa_mainloop_api *api = pa_glib_mainloop_get_api(mainloop);
    context = pa_context_new(api, "BlackLine Settings");
    pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

GtkWidget *input_volume_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Input Volume (Microphone)</b>");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    input_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(input_scale), TRUE);
    g_signal_connect(input_scale, "value-changed", G_CALLBACK(on_input_volume_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), input_scale, FALSE, FALSE, 5);

    init_pulse();

    if (context && pa_context_get_state(context) == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_get_source_info_by_name(context, NULL, source_info_callback, NULL);
        if (op) pa_operation_unref(op);
    }

    return vbox;
}