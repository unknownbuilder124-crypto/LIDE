#include "sound.h"
#include "test_sound_tab.h"
#include "output/output.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <poll.h>

/*
 * sound_tab.c
 * 
 * Sound settings UI tab implementation
 * Provides controls for input/output device selection and volume adjustment.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

static GtkWidget *volume_scale = NULL;
static GtkWidget *mute_check = NULL;
static GtkWidget *volume_label = NULL;

static GtkWidget *input_scale = NULL;
static GtkWidget *input_mute_check = NULL;
static GtkWidget *input_label = NULL;

static snd_mixer_t *mixer_handle = NULL;
static snd_mixer_elem_t *master_elem = NULL;
static guint mixer_event_source = 0;

/**
 * Executes a system command and returns the output.
 */
static char* exec_command(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    char buffer[256];
    char *result = NULL;
    size_t total = 0;
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strlen(buffer);
        result = realloc(result, total + len + 1);
        if (result) {
            memcpy(result + total, buffer, len);
            total += len;
            result[total] = '\0';
        }
    }
    
    pclose(fp);
    return result;
}

/**
 * Gets current output volume percentage from amixer.
 */
static int get_current_volume(void) {
    char *output = exec_command("amixer get Master 2>/dev/null | grep -oP '\\d+%' | head -1 | tr -d '%'");
    if (!output || strlen(output) == 0) {
        free(output);
        return 50;
    }
    
    int vol = atoi(output);
    free(output);
    return vol;
}

/**
 * Gets current output mute status from amixer.
 */
static int get_current_mute(void) {
    char *output = exec_command("amixer get Master 2>/dev/null | grep -o '\\[off\\]' | head -1");
    int muted = (output && strlen(output) > 0);
    free(output);
    return muted;
}

/**
 * Sets output volume using amixer.
 */
static void set_volume(int percent) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "amixer set Master %d%% > /dev/null 2>&1", percent);
    system(cmd);
    
    if (volume_label) {
        char *text = g_strdup_printf("Volume: %d%%", percent);
        gtk_label_set_text(GTK_LABEL(volume_label), text);
        g_free(text);
    }
}

/**
 * Toggles output mute using amixer.
 */
static void toggle_mute(GtkToggleButton *check, gpointer data) {
    (void)data;
    if (gtk_toggle_button_get_active(check)) {
        system("amixer set Master mute > /dev/null 2>&1");
    } else {
        system("amixer set Master unmute > /dev/null 2>&1");
    }
}

/**
 * Callback for output volume scale changes.
 */
static void on_volume_changed(GtkRange *range, gpointer data) {
    (void)data;
    int val = (int)gtk_range_get_value(range);
    set_volume(val);
}

/**
 * Gets current input volume percentage from amixer.
 */
static int get_current_input_volume(void) {
    char *output = exec_command("amixer get Capture 2>/dev/null | grep -oP '\\d+%' | head -1 | tr -d '%'");
    if (!output || strlen(output) == 0) {
        free(output);
        return 50;
    }
    
    int vol = atoi(output);
    free(output);
    return vol;
}

/**
 * Gets current input mute status from amixer.
 */
static int get_current_input_mute(void) {
    char *output = exec_command("amixer get Capture 2>/dev/null | grep -o '\\[off\\]' | head -1");
    int muted = (output && strlen(output) > 0);
    free(output);
    return muted;
}

/**
 * Sets input volume using amixer.
 */
static void set_input_volume(int percent) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "amixer set Capture %d%% > /dev/null 2>&1", percent);
    system(cmd);
    
    if (input_label) {
        char *text = g_strdup_printf("Input Volume: %d%%", percent);
        gtk_label_set_text(GTK_LABEL(input_label), text);
        g_free(text);
    }
}

/**
 * Toggles input mute using amixer.
 */
static void toggle_input_mute(GtkToggleButton *check, gpointer data) {
    (void)data;
    if (gtk_toggle_button_get_active(check)) {
        system("amixer set Capture mute > /dev/null 2>&1");
    } else {
        system("amixer set Capture unmute > /dev/null 2>&1");
    }
}

/**
 * Callback for input volume scale changes.
 */
static void on_input_volume_changed(GtkRange *range, gpointer data) {
    (void)data;
    int val = (int)gtk_range_get_value(range);
    set_input_volume(val);
}

/**
 * Refreshes the volume displays.
 */
static void refresh_volume_display(void) {
    /* Output */
    if (volume_scale) {
        int vol = get_current_volume();
        /* Block signals to avoid recursive updates */
        g_signal_handlers_block_by_func(volume_scale, G_CALLBACK(on_volume_changed), NULL);
        gtk_range_set_value(GTK_RANGE(volume_scale), vol);
        g_signal_handlers_unblock_by_func(volume_scale, G_CALLBACK(on_volume_changed), NULL);
        
        if (volume_label) {
            char *text = g_strdup_printf("Volume: %d%%", vol);
            gtk_label_set_text(GTK_LABEL(volume_label), text);
            g_free(text);
        }
    }
    
    if (mute_check) {
        int muted = get_current_mute();
        g_signal_handlers_block_by_func(mute_check, G_CALLBACK(toggle_mute), NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mute_check), muted);
        g_signal_handlers_unblock_by_func(mute_check, G_CALLBACK(toggle_mute), NULL);
    }
    
    /* Input */
    if (input_scale) {
        int vol = get_current_input_volume();
        g_signal_handlers_block_by_func(input_scale, G_CALLBACK(on_input_volume_changed), NULL);
        gtk_range_set_value(GTK_RANGE(input_scale), vol);
        g_signal_handlers_unblock_by_func(input_scale, G_CALLBACK(on_input_volume_changed), NULL);
        
        if (input_label) {
            char *text = g_strdup_printf("Input Volume: %d%%", vol);
            gtk_label_set_text(GTK_LABEL(input_label), text);
            g_free(text);
        }
    }
    
    if (input_mute_check) {
        int muted = get_current_input_mute();
        g_signal_handlers_block_by_func(input_mute_check, G_CALLBACK(toggle_input_mute), NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(input_mute_check), muted);
        g_signal_handlers_unblock_by_func(input_mute_check, G_CALLBACK(toggle_input_mute), NULL);
    }
}

/**
 * ALSA mixer event callback - triggered when hardware volume changes.
 */
static int mixer_event_handler(GIOChannel *source, GIOCondition condition, gpointer data) {
    (void)source;
    (void)condition;
    (void)data;
    
    if (mixer_handle) {
        snd_mixer_handle_events(mixer_handle);
        refresh_volume_display();
    }
    return TRUE;
}

/**
 * Initializes ALSA mixer and sets up event monitoring for hardware volume changes.
 */
static void init_alsa_monitor(void) {
    int err;
    
    /* Open ALSA mixer */
    err = snd_mixer_open(&mixer_handle, 0);
    if (err < 0) {
        return;
    }
    
    /* Attach to default sound card */
    err = snd_mixer_attach(mixer_handle, "default");
    if (err < 0) {
        snd_mixer_close(mixer_handle);
        mixer_handle = NULL;
        return;
    }
    
    /* Register mixer elements */
    err = snd_mixer_selem_register(mixer_handle, NULL, NULL);
    if (err < 0) {
        snd_mixer_close(mixer_handle);
        mixer_handle = NULL;
        return;
    }
    
    /* Load mixer elements */
    err = snd_mixer_load(mixer_handle);
    if (err < 0) {
        snd_mixer_close(mixer_handle);
        mixer_handle = NULL;
        return;
    }
    
    /* Find Master control element */
    master_elem = snd_mixer_first_elem(mixer_handle);
    while (master_elem) {
        if (snd_mixer_selem_is_enumerated(master_elem)) {
            master_elem = snd_mixer_elem_next(master_elem);
            continue;
        }
        
        const char *name = snd_mixer_selem_get_name(master_elem);
        if (name && (strcmp(name, "Master") == 0 || strcmp(name, "PCM") == 0)) {
            break;
        }
        master_elem = snd_mixer_elem_next(master_elem);
    }
    
    /* Get file descriptor for ALSA events */
    int fd = snd_mixer_poll_descriptors_count(mixer_handle);
    if (fd > 0) {
        struct pollfd pfds[fd];
        fd = snd_mixer_poll_descriptors(mixer_handle, pfds, fd);
        
        /* Set up GIOChannel to monitor ALSA events */
        GIOChannel *channel = g_io_channel_unix_new(pfds[0].fd);
        mixer_event_source = g_io_add_watch(channel, G_IO_IN | G_IO_PRI, mixer_event_handler, NULL);
        g_io_channel_unref(channel);
    }
}

/**
 * Timer callback to refresh volume display.
 */
static gboolean refresh_timer(gpointer data) {
    (void)data;
    refresh_volume_display();
    return TRUE;
}

/**
 * Output volume UI widget (local to sound tab).
 */
static GtkWidget* output_volume_ui_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Output</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    GtkWidget *vol_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), vol_box, FALSE, FALSE, 5);
    
    GtkWidget *vol_icon = gtk_label_new("🔊");
    gtk_box_pack_start(GTK_BOX(vol_box), vol_icon, FALSE, FALSE, 0);
    
    volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(volume_scale), FALSE);
    gtk_widget_set_size_request(volume_scale, 300, -1);
    g_signal_connect(volume_scale, "value-changed", G_CALLBACK(on_volume_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vol_box), volume_scale, TRUE, TRUE, 0);
    
    volume_label = gtk_label_new("Volume: 50%");
    gtk_box_pack_start(GTK_BOX(vol_box), volume_label, FALSE, FALSE, 0);
    
    mute_check = gtk_check_button_new_with_label("Mute");
    gtk_box_pack_start(GTK_BOX(vbox), mute_check, FALSE, FALSE, 5);
    g_signal_connect(mute_check, "toggled", G_CALLBACK(toggle_mute), NULL);
    
    return vbox;
}

/**
 * Input volume UI widget (local to sound tab).
 */
static GtkWidget* input_volume_ui_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Input (Microphone)</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    GtkWidget *vol_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), vol_box, FALSE, FALSE, 5);
    
    GtkWidget *mic_icon = gtk_label_new("🎤");
    gtk_box_pack_start(GTK_BOX(vol_box), mic_icon, FALSE, FALSE, 0);
    
    input_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(input_scale), FALSE);
    gtk_widget_set_size_request(input_scale, 300, -1);
    g_signal_connect(input_scale, "value-changed", G_CALLBACK(on_input_volume_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vol_box), input_scale, TRUE, TRUE, 0);
    
    input_label = gtk_label_new("Input Volume: 50%");
    gtk_box_pack_start(GTK_BOX(vol_box), input_label, FALSE, FALSE, 0);
    
    input_mute_check = gtk_check_button_new_with_label("Mute Microphone");
    gtk_box_pack_start(GTK_BOX(vbox), input_mute_check, FALSE, FALSE, 5);
    g_signal_connect(input_mute_check, "toggled", G_CALLBACK(toggle_input_mute), NULL);
    
    return vbox;
}

/**
 * Input device widget.
 */
static GtkWidget* input_device_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    char *cards = exec_command("cat /proc/asound/cards 2>/dev/null | grep -v '---' | grep -v 'card' | head -2");
    char *device_text;
    
    if (cards && strlen(cards) > 0) {
        device_text = g_strdup_printf("Input devices:\n%s", cards);
        free(cards);
    } else {
        device_text = g_strdup("Default input device");
    }
    
    GtkWidget *info_label = gtk_label_new(device_text);
    gtk_label_set_xalign(GTK_LABEL(info_label), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(info_label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 5);
    g_free(device_text);
    
    GtkWidget *note = gtk_label_new("(To change input device, use 'alsamixer' and press F4)");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0);
    gtk_label_set_markup(GTK_LABEL(note), "<small>(To change input device, use 'alsamixer' and press F4)</small>");
    gtk_box_pack_start(GTK_BOX(vbox), note, FALSE, FALSE, 5);
    
    return vbox;
}

/**
 * Volume levels widget.
 */
static GtkWidget* volume_levels_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>System Sounds</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    GtkWidget *note = gtk_label_new("System sounds follow master volume settings above.");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), note, FALSE, FALSE, 5);
    
    return vbox;
}

/**
 * Alert sounds widget.
 */
static GtkWidget* alert_sounds_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Alert Sounds</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    GtkWidget *check = gtk_check_button_new_with_label("Enable system beep");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
    g_signal_connect_swapped(check, "toggled", G_CALLBACK(system), "xset b on > /dev/null 2>&1");
    gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, FALSE, 5);
    
    GtkWidget *test_alert_btn = gtk_button_new_with_label("Test Alert");
    gtk_widget_set_margin_top(test_alert_btn, 5);
    g_signal_connect_swapped(test_alert_btn, "clicked", G_CALLBACK(system), "echo -e '\\a' > /dev/tty 2>/dev/null &");
    gtk_box_pack_start(GTK_BOX(vbox), test_alert_btn, FALSE, FALSE, 5);
    
    return vbox;
}

/**
 * Creates the sound settings tab with test tab.
 */
GtkWidget *sound_tab_new(void) {
    GtkWidget *notebook = gtk_notebook_new();
    
    /* Settings tab */
    GtkWidget *settings_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(settings_vbox), 10);
    
    /* Output section */
    GtkWidget *output_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(output_frame), "Output");
    gtk_box_pack_start(GTK_BOX(settings_vbox), output_frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(output_frame), output_volume_ui_widget_new());
    
    /* Output device section */
    GtkWidget *output_device_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(output_device_frame), "Output Device");
    gtk_box_pack_start(GTK_BOX(settings_vbox), output_device_frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(output_device_frame), output_device_selector_widget_new());
    
    /* Input section with manual controls */
    GtkWidget *input_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(input_frame), "Input (Microphone)");
    gtk_box_pack_start(GTK_BOX(settings_vbox), input_frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(input_frame), input_volume_ui_widget_new());
    
    /* Input device section */
    GtkWidget *input_device_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(input_device_frame), "Input Device");
    gtk_box_pack_start(GTK_BOX(settings_vbox), input_device_frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(input_device_frame), input_device_widget_new());
    
    /* System sounds section */
    GtkWidget *sounds_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(sounds_frame), "System Sounds");
    gtk_box_pack_start(GTK_BOX(settings_vbox), sounds_frame, FALSE, FALSE, 0);
    GtkWidget *sounds_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(sounds_frame), sounds_vbox);
    gtk_box_pack_start(GTK_BOX(sounds_vbox), volume_levels_widget_new(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sounds_vbox), alert_sounds_widget_new(), FALSE, FALSE, 0);
    
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(settings_vbox), spacer, TRUE, TRUE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), settings_vbox, gtk_label_new("Settings"));
    
    /* Test tab */
    GtkWidget *test_tab = test_sound_tab_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), test_tab, gtk_label_new("Visual Test"));
    
    /* Initialize ALSA event monitoring for hardware volume changes */
    init_alsa_monitor();
    
    /* Start refresh timer as fallback */
    g_timeout_add_seconds(2, refresh_timer, NULL);
    
    /* Initial refresh */
    refresh_volume_display();
    
    return notebook;
}