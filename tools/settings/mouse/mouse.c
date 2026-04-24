#include "mouse.h"
#include "mouse_style.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gdk/gdk.h>

/*
 * mouse_settings.c
 *
 * Mouse settings tab providing:
 * - Pointer speed (acceleration multiplier)
 * - Mouse acceleration on/off
 * - Scroll direction (traditional / natural)
 * - Mouse cursor style (10+ cursor themes)
 *
 * Uses xinput commands to apply changes in real time.
 */

static GtkWidget *speed_scale = NULL;
static GtkWidget *accel_check = NULL;
static GtkWidget *scroll_traditional = NULL;
static GtkWidget *scroll_natural = NULL;
static GtkWidget *status_label = NULL;

/**
 * Helper: execute system command and return output (optional)
 */
static void run_command(const char *cmd)
{
    int ret = system(cmd);
    if (ret != 0)
        g_warning("Command failed: %s", cmd);
}

/**
 * Get current pointer acceleration speed (as integer 0-100 mapping)
 * Uses `xinput list-props` to get "Device Accel Velocity Scaling".
 * Fallback: 50 if detection fails.
 */
static int get_current_speed(void)
{
    FILE *fp = popen("xinput list-props \"Virtual core pointer\" 2>/dev/null | grep -i \"Accel Speed\" | awk -F': ' '{print $2}' | head -1", "r");
    if (!fp) return 50;
    char buf[32] = {0};
    if (fgets(buf, sizeof(buf), fp)) {
        double val = atof(buf);
        pclose(fp);
        /* val range is typically -1..1; map -1->0, 1->100 */
        return (int)((val + 1.0) / 2.0 * 100.0);
    }
    pclose(fp);
    return 50;
}

/**
 * Get current acceleration enabled/disabled status.
 * xinput property "Device Enabled" for pointer? No, we check "Device Accel Profile".
 */
static gboolean get_accel_enabled(void)
{
    FILE *fp = popen("xinput list-props \"Virtual core pointer\" 2>/dev/null | grep -i \"Device Accel Profile\" | awk -F': ' '{print $2}' | head -1", "r");
    if (!fp) return FALSE;
    char buf[32] = {0};
    if (fgets(buf, sizeof(buf), fp)) {
        int val = atoi(buf);
        pclose(fp);
        /* Profile 0 = flat (no accel), 1 = classic (accel) */
        return (val == 1);
    }
    pclose(fp);
    return TRUE;
}

/**
 * Get current scroll direction (natural or traditional).
 * Property "Natural Scrolling Enabled" (1 = natural, 0 = traditional).
 */
static gboolean get_natural_scroll(void)
{
    FILE *fp = popen("xinput list-props \"Virtual core pointer\" 2>/dev/null | grep -i \"Natural Scrolling\" | awk -F': ' '{print $2}' | head -1", "r");
    if (!fp) return FALSE;
    char buf[32] = {0};
    if (fgets(buf, sizeof(buf), fp)) {
        int val = atoi(buf);
        pclose(fp);
        return (val == 1);
    }
    pclose(fp);
    return FALSE;
}

/**
 * Apply pointer speed (0-100) to system.
 * Convert 0-100 -> -1..1 range for xinput.
 */
static void apply_speed(int percent)
{
    double speed = (percent / 50.0) - 1.0;  // 0 -> -1, 100 -> 1
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "xinput set-prop \"Virtual core pointer\" \"libinput Accel Speed\" %.2f 2>/dev/null", speed);
    run_command(cmd);
    /* Also fallback to "Device Accel Velocity Scaling" if libinput not used */
    snprintf(cmd, sizeof(cmd), "xinput set-prop \"Virtual core pointer\" \"Device Accel Velocity Scaling\" %.2f 2>/dev/null", speed);
    run_command(cmd);
}

/**
 * Apply acceleration on/off.
 * For libinput: Accel Profile Enabled (0 = flat, 1 = adaptive)
 */
static void apply_accel(gboolean enabled)
{
    char cmd[256];
    if (enabled) {
        /* Enable acceleration: set profile to adaptive (1) */
        snprintf(cmd, sizeof(cmd), "xinput set-prop \"Virtual core pointer\" \"libinput Accel Profile Enabled\" 1 2>/dev/null");
    } else {
        /* Disable acceleration: set profile to flat (0) */
        snprintf(cmd, sizeof(cmd), "xinput set-prop \"Virtual core pointer\" \"libinput Accel Profile Enabled\" 0 2>/dev/null");
    }
    run_command(cmd);
    /* Also fallback for non-libinput drivers */
    if (!enabled) {
        run_command("xinput set-prop \"Virtual core pointer\" \"Device Accel Profile\" 0 2>/dev/null");
    } else {
        run_command("xinput set-prop \"Virtual core pointer\" \"Device Accel Profile\" 1 2>/dev/null");
    }
}

/**
 * Apply scroll direction.
 * @param natural TRUE for natural (reverse), FALSE for traditional.
 */
static void apply_scroll_direction(gboolean natural)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "xinput set-prop \"Virtual core pointer\" \"Natural Scrolling Enabled\" %d 2>/dev/null", natural ? 1 : 0);
    run_command(cmd);
}

/**
 * Callback for speed slider changes.
 */
static void on_speed_changed(GtkRange *range, gpointer data)
{
    (void)data;
    int val = (int)gtk_range_get_value(range);
    apply_speed(val);
    if (status_label) {
        char *msg = g_strdup_printf("Pointer speed set to %d%%", val);
        gtk_label_set_text(GTK_LABEL(status_label), msg);
        g_free(msg);
    }
}

/**
 * Callback for acceleration checkbox toggled.
 */
static void on_accel_toggled(GtkToggleButton *check, gpointer data)
{
    (void)data;
    gboolean enabled = gtk_toggle_button_get_active(check);
    apply_accel(enabled);
    if (status_label) {
        const char *msg = enabled ? "Mouse acceleration enabled" : "Mouse acceleration disabled";
        gtk_label_set_text(GTK_LABEL(status_label), msg);
    }
}

/**
 * Callback for scroll direction radio button toggled.
 */
static void on_scroll_direction_toggled(GtkToggleButton *radio, gpointer data)
{
    if (!gtk_toggle_button_get_active(radio)) return;
    gboolean natural = (radio == GTK_TOGGLE_BUTTON(scroll_natural));
    apply_scroll_direction(natural);
    if (status_label) {
        const char *msg = natural ? "Natural scrolling enabled" : "Traditional scrolling enabled";
        gtk_label_set_text(GTK_LABEL(status_label), msg);
    }
}

/**
 * Refresh UI to match current system settings.
 */
static void refresh_ui(void)
{
    if (speed_scale) {
        int speed = get_current_speed();
        g_signal_handlers_block_by_func(speed_scale, G_CALLBACK(on_speed_changed), NULL);
        gtk_range_set_value(GTK_RANGE(speed_scale), speed);
        g_signal_handlers_unblock_by_func(speed_scale, G_CALLBACK(on_speed_changed), NULL);
    }
    if (accel_check) {
        gboolean accel = get_accel_enabled();
        g_signal_handlers_block_by_func(accel_check, G_CALLBACK(on_accel_toggled), NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(accel_check), accel);
        g_signal_handlers_unblock_by_func(accel_check, G_CALLBACK(on_accel_toggled), NULL);
    }
    if (scroll_traditional && scroll_natural) {
        gboolean natural = get_natural_scroll();
        g_signal_handlers_block_by_func(scroll_traditional, G_CALLBACK(on_scroll_direction_toggled), NULL);
        g_signal_handlers_block_by_func(scroll_natural, G_CALLBACK(on_scroll_direction_toggled), NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scroll_natural), natural);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scroll_traditional), !natural);
        g_signal_handlers_unblock_by_func(scroll_traditional, G_CALLBACK(on_scroll_direction_toggled), NULL);
        g_signal_handlers_unblock_by_func(scroll_natural, G_CALLBACK(on_scroll_direction_toggled), NULL);
    }
}

/* Timer to periodically refresh UI (in case external changes occur) */
static gboolean refresh_timer(gpointer data)
{
    (void)data;
    refresh_ui();
    return TRUE;
}

/**
 * Creates the mouse settings tab.
 */
GtkWidget *mouse_settings_tab_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);

    /* ---- Pointer Speed ---- */
    GtkWidget *speed_frame = gtk_frame_new("Pointer Speed");
    GtkWidget *speed_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(speed_box), 10);
    gtk_container_add(GTK_CONTAINER(speed_frame), speed_box);

    /* Speed label row: "Slow" and "Fast" labels at ends */
    GtkWidget *speed_labels = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *slow_label = gtk_label_new("Slow");
    gtk_widget_set_halign(slow_label, GTK_ALIGN_START);
    GtkWidget *fast_label = gtk_label_new("Fast");
    gtk_widget_set_halign(fast_label, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(speed_labels), slow_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(speed_labels), fast_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(speed_box), speed_labels, FALSE, FALSE, 0);

    /* Speed slider */
    speed_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(speed_scale), TRUE);
    gtk_widget_set_size_request(speed_scale, 300, -1);
    g_signal_connect(speed_scale, "value-changed", G_CALLBACK(on_speed_changed), NULL);
    gtk_box_pack_start(GTK_BOX(speed_box), speed_scale, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), speed_frame, FALSE, FALSE, 0);

    /* ---- Mouse Acceleration ---- */
    GtkWidget *accel_frame = gtk_frame_new("Mouse Acceleration");
    GtkWidget *accel_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(accel_box), 10);
    gtk_container_add(GTK_CONTAINER(accel_frame), accel_box);

    accel_check = gtk_check_button_new_with_label("Recommended for most users and applications");
    g_signal_connect(accel_check, "toggled", G_CALLBACK(on_accel_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(accel_box), accel_check, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), accel_frame, FALSE, FALSE, 0);

    /* ---- Scroll Direction ---- */
    GtkWidget *scroll_frame = gtk_frame_new("Scroll Direction");
    GtkWidget *scroll_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(scroll_box), 10);
    gtk_container_add(GTK_CONTAINER(scroll_frame), scroll_box);

    /* Traditional scroll radio */
    scroll_traditional = gtk_radio_button_new_with_label(NULL, "Traditional");
    gtk_widget_set_tooltip_text(scroll_traditional, "Scrolling moves the view");
    /* Natural scroll radio (reversed) */
    scroll_natural = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(scroll_traditional), "Natural");
    gtk_widget_set_tooltip_text(scroll_natural, "Scrolling moves the content");

    g_signal_connect(scroll_traditional, "toggled", G_CALLBACK(on_scroll_direction_toggled), NULL);
    g_signal_connect(scroll_natural, "toggled", G_CALLBACK(on_scroll_direction_toggled), NULL);

    gtk_box_pack_start(GTK_BOX(scroll_box), scroll_traditional, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(scroll_box), scroll_natural, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), scroll_frame, FALSE, FALSE, 0);

    /* ---- Separator ---- */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 10);

    /* ---- Mouse Cursor Style (from mouse_style.c) ---- */
    GtkWidget *style_widget = mouse_style_widget_new();
    gtk_box_pack_start(GTK_BOX(vbox), style_widget, FALSE, FALSE, 0);

    /* ---- Status label for feedback ---- */
    status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

    /* Spacer to push everything up */
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), spacer, TRUE, TRUE, 0);

    /* Initial refresh of UI */
    refresh_ui();

    /* Periodic refresh every 2 seconds in case external tools change settings */
    g_timeout_add_seconds(2, refresh_timer, NULL);

    return vbox;
}