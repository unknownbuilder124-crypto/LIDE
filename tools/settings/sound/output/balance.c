/*
 * balance.c
 * 
 * Audio channel balance control (left/right/center)
 * Adjusts stereo balance for playback devices.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

#define _USE_MATH_DEFINES
#include "output.h"
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif

/**
 * Balance control data structure.
 *
 * Holds all necessary state for balance control widget including
 * PulseAudio context references and UI component handles.
 */
typedef struct {
    pa_context *context;           /* PulseAudio context for volume operations */
    GtkWidget *balance_scale;      /* GTK scale widget for balance control */
    GtkWidget *balance_label;      /* Label showing current balance value */
    guint refresh_timeout_id;      /* Timer ID for periodic refresh (0 if none) */
    double balance;                /* Current balance value (-100 to +100) */
    char *default_sink_name;       /* Cached default sink name */
} BalanceData;

static BalanceData *balance_data = NULL;

/* Forward declarations */
static void on_balance_changed(GtkRange *range, gpointer data);
static void balance_apply_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
static void get_default_sink_name(void);

/**
 * Converts balance percentage to left/right channel volumes.
 *
 * Balance mapping algorithm: preserves total power (RMS) across both channels
 * while shifting perceived center. Uses equal-power panning law.
 *
 * @param balance Balance value (-100 to +100):
 *                -100 = 100% left, 0% right
 *                   0 = 70.7% left, 70.7% right (-3dB each, total power preserved)
 *                +100 = 0% left, 100% right
 * @param left    Output pointer for left channel volume (0.0 to 1.0)
 * @param right   Output pointer for right channel volume (0.0 to 1.0)
 *
 * @return 0 on success, -1 on invalid input.
 *
 * @note Uses constant-power panning: left^2 + right^2 = 1.0
 *       This ensures perceived loudness remains constant during panning.
 */
static int balance_to_volumes(double balance, double *left, double *right) {
    if (!left || !right) return -1;
    
    /* Clamp balance to valid range [-100, 100] */
    if (balance < -100.0) balance = -100.0;
    if (balance > 100.0) balance = 100.0;
    
    /* Convert balance (-100..100) to pan angle (-45..+45 degrees) */
    double angle = (balance / 100.0) * 45.0 * (M_PI / 180.0);
    
    /* Constant-power panning law */
    *left = cos(M_PI_4 + angle);
    *right = sin(M_PI_4 + angle);
    
    /* Clamp to [0, 1] range */
    if (*left < 0) *left = 0;
    if (*left > 1) *left = 1;
    if (*right < 0) *right = 0;
    if (*right > 1) *right = 1;
    
    return 0;
}

/**
 * Converts left/right channel volumes to balance percentage.
 */
static double volumes_to_balance(double left, double right) {
    if (left <= 0.0 || right <= 0.0) return 0.0;
    
    double angle = atan2(right, left);
    double balance = (angle - M_PI_4) * (100.0 / M_PI_4);
    
    if (balance < -100.0) balance = -100.0;
    if (balance > 100.0) balance = 100.0;
    
    return balance;
}

/**
 * Callback structure for balance apply operation.
 */
typedef struct {
    BalanceData *data;
    double balance;
} BalanceApplyData;

/**
 * Callback for applying balance after retrieving current volume.
 */
static void balance_apply_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    BalanceApplyData *apply_data = (BalanceApplyData *)userdata;
    
    if (eol < 0 || !i || i->volume.channels < 2) {
        if (apply_data) free(apply_data);
        return;
    }
    
    double current_vol = (double)pa_cvolume_avg(&i->volume) / PA_VOLUME_NORM;
    
    double left_vol, right_vol;
    balance_to_volumes(apply_data->balance, &left_vol, &right_vol);
    
    pa_cvolume cv;
    pa_cvolume_set(&cv, 2, PA_VOLUME_NORM);
    cv.values[0] = (pa_volume_t)(left_vol * current_vol * PA_VOLUME_NORM);
    cv.values[1] = (pa_volume_t)(right_vol * current_vol * PA_VOLUME_NORM);
    
    /* Use the sink name from the info structure */
    if (i->name) {
        pa_operation *set_op = pa_context_set_sink_volume_by_name(c, i->name, &cv, NULL, NULL);
        if (set_op) pa_operation_unref(set_op);
    }
    
    free(apply_data);
}

/**
 * Applies balance setting to PulseAudio sink.
 */
static void apply_balance(BalanceData *data, double balance) {
    if (!data || !data->context) return;
    
    pa_context_state_t state = pa_context_get_state(data->context);
    if (state != PA_CONTEXT_READY) return;
    
    BalanceApplyData *apply_data = malloc(sizeof(BalanceApplyData));
    if (!apply_data) return;
    
    apply_data->data = data;
    apply_data->balance = balance;
    
    /* Use NULL to get the default sink */
    pa_operation *op = pa_context_get_sink_info_by_name(data->context, NULL, 
        balance_apply_callback, apply_data);
    if (op) pa_operation_unref(op);
}

/**
 * GTK callback for balance scale value changes.
 */
static void on_balance_changed(GtkRange *range, gpointer data) {
    BalanceData *balance_data_ptr = (BalanceData *)data;
    if (!balance_data_ptr) return;
    
    double balance = gtk_range_get_value(range);
    balance_data_ptr->balance = balance;
    apply_balance(balance_data_ptr, balance);
}

/**
 * PulseAudio sink information callback for balance widget.
 */
static void balance_sink_info_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    (void)c;
    BalanceData *data = (BalanceData *)userdata;
    
    if (eol < 0 || !data || !i) return;
    
    /* Cache the sink name */
    if (i->name && !data->default_sink_name) {
        data->default_sink_name = strdup(i->name);
    }
    
    if (i->volume.channels >= 2 && data->balance_scale && GTK_IS_RANGE(data->balance_scale)) {
        double left = (double)i->volume.values[0] / PA_VOLUME_NORM;
        double right = (double)i->volume.values[1] / PA_VOLUME_NORM;
        
        double balance = volumes_to_balance(left, right);
        data->balance = balance;
        
        g_signal_handlers_block_by_func(data->balance_scale, G_CALLBACK(on_balance_changed), data);
        gtk_range_set_value(GTK_RANGE(data->balance_scale), balance);
        g_signal_handlers_unblock_by_func(data->balance_scale, G_CALLBACK(on_balance_changed), data);
        
        if (data->balance_label && GTK_IS_LABEL(data->balance_label)) {
            char *text = g_strdup_printf("Balance: %.0f%% %s", 
                fabs(balance),
                balance < -5 ? "Left" : (balance > 5 ? "Right" : "Center"));
            gtk_label_set_text(GTK_LABEL(data->balance_label), text);
            g_free(text);
        }
    }
}

/**
 * Periodic refresh callback for balance widget.
 */
static gboolean refresh_balance_timer(gpointer data) {
    BalanceData *balance_data_ptr = (BalanceData *)data;
    if (!balance_data_ptr || !balance_data_ptr->context) return TRUE;
    
    pa_context_state_t state = pa_context_get_state(balance_data_ptr->context);
    if (state == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_get_sink_info_by_name(balance_data_ptr->context, NULL, 
            balance_sink_info_callback, balance_data_ptr);
        if (op) pa_operation_unref(op);
    }
    
    return TRUE;
}

/**
 * PulseAudio context state change callback for balance module.
 */
static void balance_context_state_callback(pa_context *c, void *userdata) {
    BalanceData *data = (BalanceData *)userdata;
    if (!data) return;
    
    pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_READY) {
        pa_operation *op = pa_context_get_sink_info_by_name(c, NULL, 
            balance_sink_info_callback, data);
        if (op) pa_operation_unref(op);
    } else if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
        /* Handle disconnection */
    }
}

/**
 * Initializes PulseAudio context for balance control.
 */
static void init_balance_pulse(BalanceData *data) {
    if (!data || data->context) return;
    
    pa_glib_mainloop *mainloop = pa_glib_mainloop_new(g_main_context_default());
    pa_mainloop_api *api = pa_glib_mainloop_get_api(mainloop);
    data->context = pa_context_new(api, "BlackLine Balance Control");
    pa_context_set_state_callback(data->context, balance_context_state_callback, data);
    pa_context_connect(data->context, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

/**
 * Creates the audio balance control widget.
 *
 * @return GtkWidget containing the balance control UI.
 */
GtkWidget *balance_widget_new(void) {
    if (!balance_data) {
        balance_data = g_new0(BalanceData, 1);
        balance_data->balance_scale = NULL;
        balance_data->balance_label = NULL;
        balance_data->refresh_timeout_id = 0;
        balance_data->context = NULL;
        balance_data->balance = 0.0;
        balance_data->default_sink_name = NULL;
    }
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    GtkWidget *balance_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), balance_box, FALSE, FALSE, 5);
    
    GtkWidget *left_label = gtk_label_new("L");
    gtk_box_pack_start(GTK_BOX(balance_box), left_label, FALSE, FALSE, 0);
    
    balance_data->balance_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -100, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(balance_data->balance_scale), FALSE);
    gtk_widget_set_size_request(balance_data->balance_scale, 300, -1);
    g_signal_connect(balance_data->balance_scale, "value-changed", 
                     G_CALLBACK(on_balance_changed), balance_data);
    gtk_box_pack_start(GTK_BOX(balance_box), balance_data->balance_scale, TRUE, TRUE, 0);
    
    GtkWidget *right_label = gtk_label_new("R");
    gtk_box_pack_start(GTK_BOX(balance_box), right_label, FALSE, FALSE, 0);
    
    balance_data->balance_label = gtk_label_new("Balance: Center");
    gtk_label_set_xalign(GTK_LABEL(balance_data->balance_label), 0.0);
    gtk_widget_set_margin_top(balance_data->balance_label, 5);
    gtk_box_pack_start(GTK_BOX(vbox), balance_data->balance_label, FALSE, FALSE, 0);
    
    init_balance_pulse(balance_data);
    balance_data->refresh_timeout_id = g_timeout_add_seconds(2, refresh_balance_timer, balance_data);
    
    return vbox;
}

/**
 * Cleans up balance control resources.
 */
void balance_cleanup(void) {
    if (!balance_data) return;
    
    if (balance_data->refresh_timeout_id > 0) {
        g_source_remove(balance_data->refresh_timeout_id);
        balance_data->refresh_timeout_id = 0;
    }
    
    if (balance_data->context) {
        pa_context_disconnect(balance_data->context);
        pa_context_unref(balance_data->context);
        balance_data->context = NULL;
    }
    
    if (balance_data->default_sink_name) {
        free(balance_data->default_sink_name);
        balance_data->default_sink_name = NULL;
    }
    
    g_free(balance_data);
    balance_data = NULL;
}