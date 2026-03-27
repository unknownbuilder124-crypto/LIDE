#include "test_sound_tab.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static GtkWidget *waveform_drawing_area = NULL;
static GtkWidget *volume_meter = NULL;
static GtkWidget *status_label = NULL;
static guint animation_timer = 0;
static float animation_phase = 0.0;
static int test_running = 0;
static pid_t test_pid = 0;

/* Forward declaration */
static gboolean stop_sound_test(void);

/**
 * Draws a waveform visualization on the drawing area.
 */
static gboolean draw_waveform(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)data;
    
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    
    int width = alloc.width;
    int height = alloc.height;
    
    /* Clear background */
    cairo_set_source_rgb(cr, 0.11, 0.14, 0.20);
    cairo_paint(cr);
    
    if (test_running) {
        /* Draw animated sine wave */
        cairo_set_source_rgb(cr, 0.0, 1.0, 0.53);
        cairo_set_line_width(cr, 2.0);
        
        double amplitude = height * 0.3;
        double center_y = height / 2;
        
        cairo_move_to(cr, 0, center_y);
        
        for (int x = 0; x < width; x++) {
            double t = (double)x / width * 4 * M_PI;
            double y = center_y + amplitude * sin(t + animation_phase);
            cairo_line_to(cr, x, y);
        }
        
        cairo_stroke(cr);
        
        /* Draw frequency markers */
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.4);
        cairo_set_line_width(cr, 0.5);
        
        for (int i = 1; i <= 3; i++) {
            double x = width / 4 * i;
            cairo_move_to(cr, x, 0);
            cairo_line_to(cr, x, height);
            cairo_stroke(cr);
        }
        
        /* Draw center line */
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.6);
        cairo_move_to(cr, 0, center_y);
        cairo_line_to(cr, width, center_y);
        cairo_stroke(cr);
        
        /* Draw "Testing" text */
        cairo_set_source_rgb(cr, 0.0, 1.0, 0.53);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, 10, 30);
        cairo_show_text(cr, "🎵 Sound Test in Progress...");
    } else {
        /* Draw idle waveform */
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.4);
        cairo_set_line_width(cr, 1.5);
        
        double center_y = height / 2;
        
        cairo_move_to(cr, 0, center_y);
        
        for (int x = 0; x < width; x++) {
            double t = (double)x / width * 2 * M_PI;
            double y = center_y + 5 * sin(t * 2);
            cairo_line_to(cr, x, y);
        }
        
        cairo_stroke(cr);
        
        /* Draw idle text */
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.6);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12);
        cairo_move_to(cr, width / 2 - 60, height / 2 - 10);
        cairo_show_text(cr, "Click 'Test Sound' to visualize");
    }
    
    return FALSE;
}

/**
 * Animates the waveform by updating the phase and redrawing.
 */
static gboolean animate_waveform(gpointer data) {
    (void)data;
    
    if (test_running) {
        animation_phase += 0.1;
        if (animation_phase > 2 * M_PI) {
            animation_phase -= 2 * M_PI;
        }
        
        if (waveform_drawing_area) {
            gtk_widget_queue_draw(waveform_drawing_area);
        }
        
        return TRUE;
    }
    
    return FALSE;
}

/**
 * Updates the volume meter visualization.
 */
static void update_volume_meter(int volume) {
    if (!volume_meter) return;
    
    GtkWidget *event_box = volume_meter;
    GtkStyleContext *context = gtk_widget_get_style_context(event_box);
    
    if (test_running) {
        char *css = g_strdup_printf(
            "#volume-meter { background-color: #00ff88; background-image: linear-gradient(to right, #00ff88 %d%%, #2a323a %d%%); border-radius: 4px; }",
            volume, volume
        );
        
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, css, -1, NULL);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
        g_free(css);
    } else {
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider,
            "#volume-meter { background-color: #2a323a; border-radius: 4px; }",
            -1, NULL);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
    }
}

/**
 * Simulates volume meter animation during sound test.
 */
static gboolean animate_volume_meter(gpointer data) {
    (void)data;
    
    static int fake_volume = 0;
    static int direction = 1;
    
    if (test_running) {
        fake_volume += direction * 5;
        if (fake_volume >= 80) direction = -1;
        if (fake_volume <= 10) direction = 1;
        
        update_volume_meter(fake_volume);
        
        char *text = g_strdup_printf("🎤 Volume Level: %d%%", fake_volume);
        gtk_label_set_text(GTK_LABEL(status_label), text);
        g_free(text);
        
        return TRUE;
    } else {
        update_volume_meter(0);
        gtk_label_set_text(GTK_LABEL(status_label), "Ready - Click 'Test Sound' to start");
        return FALSE;
    }
}

/**
 * Stops the sound test.
 */
static gboolean stop_sound_test(void) {
    if (test_running) {
        test_running = 0;
        
        /* Kill the test process if still running */
        if (test_pid > 0) {
            kill(test_pid, SIGTERM);
            test_pid = 0;
        }
        
        if (animation_timer) {
            g_source_remove(animation_timer);
            animation_timer = 0;
        }
        
        if (waveform_drawing_area) {
            gtk_widget_queue_draw(waveform_drawing_area);
        }
        
        gtk_label_set_text(GTK_LABEL(status_label), "Test complete - Click to test again");
    }
    
    return FALSE;
}

/**
 * Starts the sound test visualization.
 */
static void start_sound_test(void) {
    if (test_running) return;
    
    test_running = 1;
    animation_phase = 0;
    
    /* Play test sound */
    test_pid = fork();
    if (test_pid == 0) {
        /* Child process - play sound */
        execl("/bin/sh", "sh", "-c", "speaker-test -t sine -f 440 -l 3 2>/dev/null", NULL);
        exit(0);
    }
    
    /* Start animations */
    if (animation_timer) {
        g_source_remove(animation_timer);
    }
    animation_timer = g_timeout_add(30, animate_waveform, NULL);
    
    g_timeout_add(50, animate_volume_meter, NULL);
    
    gtk_label_set_text(GTK_LABEL(status_label), "🎵 Playing 440Hz test tone...");
    
    /* Stop test after 3 seconds */
    g_timeout_add_seconds(3, (GSourceFunc)stop_sound_test, NULL);
}

/**
 * Stops the test (cleanup).
 */
static void cleanup_test(void) {
    if (test_running) {
        test_running = 0;
        if (test_pid > 0) {
            kill(test_pid, SIGTERM);
            test_pid = 0;
        }
        if (animation_timer) {
            g_source_remove(animation_timer);
            animation_timer = 0;
        }
    }
}

/**
 * Creates the sound test visualization tab.
 */
GtkWidget *test_sound_tab_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    
    /* Title */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b><big>🎧 Sound Test Visualization</big></b>");
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    /* Waveform visualization area */
    waveform_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(waveform_drawing_area, -1, 150);
    g_signal_connect(waveform_drawing_area, "draw", G_CALLBACK(draw_waveform), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), waveform_drawing_area, FALSE, FALSE, 5);
    
    /* Volume meter visualization */
    volume_meter = gtk_event_box_new();
    gtk_widget_set_name(volume_meter, "volume-meter");
    gtk_widget_set_size_request(volume_meter, -1, 30);
    gtk_widget_set_margin_top(volume_meter, 10);
    gtk_box_pack_start(GTK_BOX(vbox), volume_meter, FALSE, FALSE, 0);
    
    /* Status label */
    status_label = gtk_label_new("Ready - Click 'Test Sound' to start");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.5);
    gtk_widget_set_margin_top(status_label, 10);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
    
    /* Test button */
    GtkWidget *test_btn = gtk_button_new_with_label("🔊 Test Sound");
    gtk_widget_set_size_request(test_btn, 150, 40);
    g_signal_connect_swapped(test_btn, "clicked", G_CALLBACK(start_sound_test), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), test_btn, FALSE, FALSE, 10);
    
    /* Frequency information */
    GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(vbox), info_box, FALSE, FALSE, 0);
    
    GtkWidget *freq_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(freq_label), "<small>Test Tone: 440Hz (A4)</small>");
    gtk_box_pack_start(GTK_BOX(info_box), freq_label, TRUE, TRUE, 0);
    
    GtkWidget *duration_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(duration_label), "<small>Duration: 3 seconds</small>");
    gtk_box_pack_start(GTK_BOX(info_box), duration_label, TRUE, TRUE, 0);
    
    /* Stop button */
    GtkWidget *stop_btn = gtk_button_new_with_label("⏹️ Stop");
    g_signal_connect_swapped(stop_btn, "clicked", G_CALLBACK(stop_sound_test), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), stop_btn, FALSE, FALSE, 5);
    
    /* Cleanup on destroy */
    g_signal_connect_swapped(vbox, "destroy", G_CALLBACK(cleanup_test), NULL);
    
    return vbox;
}