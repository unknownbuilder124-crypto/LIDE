#include "mouse_style.h"
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>

/*
 * mouse_style.c
 *
 * Provides a selection of 10+ X11 cursor themes/styles.
 * Changes are applied instantly to the whole X session.
 * Also provides cursor color and size customization with persistence.
 */

typedef struct {
    const char *name;       /* Display name */
    unsigned int shape;     /* XC_* cursor shape constant */
    const char *icon;       /* Emoji / icon hint for UI */
} CursorStyle;

static const CursorStyle cursor_styles[] = {
    {"Default (X cursor)", XC_X_cursor, "✛"},
    {"Left Pointer", XC_left_ptr, "➡️"},
    {"Hand (Link)", XC_hand2, "🖱️"},
    {"Text (I-beam)", XC_xterm, "⎜"},
    {"Crosshair", XC_crosshair, "⌗"},
    {"Working / Watch", XC_watch, "⏳"},
    {"Move", XC_fleur, "⬌"},
    {"Resize NW-SE", XC_sizing, "↖️"},
    {"Resize N-S", XC_sb_v_double_arrow, "↕️"},
    {"Resize E-W", XC_sb_h_double_arrow, "↔️"},
    {"Question arrow", XC_question_arrow, "❓"},
    {"Help", XC_question_arrow, "ℹ️"},
    {"No (Forbidden)", XC_X_cursor, "🚫"},
    {"Pen", XC_pencil, "✏️"}
};

#define NUM_STYLES (sizeof(cursor_styles) / sizeof(cursor_styles[0]))
#define CONFIG_PATH "/.config/blackline/mouse.conf"

/* Global settings */
static int cursor_size = 24;
static char cursor_color[16] = "#ffffff";
static unsigned int current_shape = XC_left_ptr;
static GtkWidget *size_scale = NULL;
static GtkWidget *color_button = NULL;
static GtkWidget *preview_label = NULL;
static GtkListBox *style_listbox = NULL;

/* Forward declarations */
static void on_size_changed(GtkRange *range, gpointer data);
static void on_style_selected(GtkListBox *listbox, GtkListBoxRow *row, gpointer user_data);

/**
 * Get config file path
 */
static char* get_config_path(void)
{
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    char *path = g_strdup_printf("%s%s", home, CONFIG_PATH);
    
    /* Ensure directory exists */
    char *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);
    
    return path;
}

/**
 * Load saved cursor settings from config file
 */
static void load_saved_settings(void)
{
    char *config_path = get_config_path();
    FILE *fp = fopen(config_path, "r");
    
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            char *key = strtok(line, "=");
            char *value = strtok(NULL, "=");
            if (key && value) {
                if (strcmp(key, "cursor_size") == 0) {
                    cursor_size = atoi(value);
                    if (cursor_size < 16) cursor_size = 16;
                    if (cursor_size > 64) cursor_size = 64;
                } else if (strcmp(key, "cursor_color") == 0) {
                    strncpy(cursor_color, value, sizeof(cursor_color) - 1);
                } else if (strcmp(key, "cursor_shape") == 0) {
                    current_shape = atoi(value);
                }
            }
        }
        fclose(fp);
    }
    g_free(config_path);
}

/**
 * Save cursor settings to config file
 */
static void save_settings(void)
{
    char *config_path = get_config_path();
    FILE *fp = fopen(config_path, "w");
    
    if (fp) {
        fprintf(fp, "cursor_size=%d\n", cursor_size);
        fprintf(fp, "cursor_color=%s\n", cursor_color);
        fprintf(fp, "cursor_shape=%u\n", current_shape);
        fclose(fp);
    }
    g_free(config_path);
}

/**
 * Apply cursor size using multiple methods
 */
static void apply_cursor_size(int size)
{
    /* Method 1: GNOME/GTK settings */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "gsettings set org.gnome.desktop.interface cursor-size %d 2>/dev/null", size);
    system(cmd);
    
    /* Method 2: Xresources */
    char xresources[512];
    snprintf(xresources, sizeof(xresources), "echo \"Xcursor.size: %d\" | xrdb -merge 2>/dev/null", size);
    system(xresources);
    
    printf("Cursor size applied: %d\n", size);
}

/**
 * Apply cursor color - creates a colored cursor using xcursor
 */
static void apply_cursor_color(const char *color)
{
    /* Remove # if present */
    char hex[16];
    strcpy(hex, color);
    if (hex[0] == '#') {
        memmove(hex, hex + 1, strlen(hex));
    }
    
    char cmd[256];
    
    /* Set cursor theme (not using the undefined 'size' variable) */
    snprintf(cmd, sizeof(cmd), "gsettings set org.gnome.desktop.interface cursor-theme 'Adwaita' 2>/dev/null");
    system(cmd);
    
    /* Use xsetroot for basic color */
    snprintf(cmd, sizeof(cmd), "xsetroot -cursor_name left_ptr -fg '#%s' -bg black 2>/dev/null", hex);
    system(cmd);
    
    printf("Cursor color applied: %s\n", color);
}

/**
 * Apply cursor style using Xlib
 */
static void apply_cursor_style(unsigned int shape)
{
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return;
    
    /* Create the cursor */
    Cursor cursor = XCreateFontCursor(dpy, shape);
    
    /* Set cursor on root window */
    XDefineCursor(dpy, DefaultRootWindow(dpy), cursor);
    
    /* Set cursor on all existing top-level windows */
    Window root = DefaultRootWindow(dpy);
    Window parent, *children;
    unsigned int nchildren;
    
    if (XQueryTree(dpy, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            XDefineCursor(dpy, children[i], cursor);
        }
        XFree(children);
    }
    
    XFlush(dpy);
    
    /* Use xsetroot as fallback */
    char cmd[256];
    const char *cursor_name = "left_ptr";
    
    /* Map shape to cursor name */
    switch(shape) {
        case XC_left_ptr: cursor_name = "left_ptr"; break;
        case XC_hand2: cursor_name = "hand2"; break;
        case XC_xterm: cursor_name = "xterm"; break;
        case XC_crosshair: cursor_name = "crosshair"; break;
        case XC_watch: cursor_name = "watch"; break;
        case XC_fleur: cursor_name = "fleur"; break;
        case XC_sizing: cursor_name = "sizing"; break;
        case XC_sb_v_double_arrow: cursor_name = "sb_v_double_arrow"; break;
        case XC_sb_h_double_arrow: cursor_name = "sb_h_double_arrow"; break;
        case XC_question_arrow: cursor_name = "question_arrow"; break;
        case XC_pencil: cursor_name = "pencil"; break;
        default: cursor_name = "left_ptr"; break;
    }
    
    snprintf(cmd, sizeof(cmd), "xsetroot -cursor_name %s 2>/dev/null", cursor_name);
    system(cmd);
    
    XCloseDisplay(dpy);
    
    /* Apply color and size after style change */
    apply_cursor_color(cursor_color);
    apply_cursor_size(cursor_size);
    
    current_shape = shape;
    save_settings();
}

/**
 * Callback for size slider changes
 */
static void on_size_changed(GtkRange *range, gpointer data)
{
    (void)data;
    int size = (int)gtk_range_get_value(range);
    cursor_size = size;
    apply_cursor_size(size);
    
    if (preview_label) {
        char *msg = g_strdup_printf("✓ Cursor size set to %dpx", size);
        gtk_label_set_text(GTK_LABEL(preview_label), msg);
        g_free(msg);
    }
    save_settings();
}

/**
 * Callback for color selection
 */
static void on_color_set(GtkColorButton *button, gpointer data)
{
    (void)data;
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    
    /* Convert to hex string */
    char hex[16];
    snprintf(hex, sizeof(hex), "#%02x%02x%02x",
             (int)(color.red * 255),
             (int)(color.green * 255),
             (int)(color.blue * 255));
    
    strncpy(cursor_color, hex, sizeof(cursor_color));
    apply_cursor_color(cursor_color);
    
    if (preview_label) {
        char *msg = g_strdup_printf("✓ Cursor color set to %s", hex);
        gtk_label_set_text(GTK_LABEL(preview_label), msg);
        g_free(msg);
    }
    save_settings();
    
    /* Re-apply current style to show color */
    apply_cursor_style(current_shape);
}

/**
 * Callback when a style is chosen from the list.
 */
static void on_style_selected(GtkListBox *listbox, GtkListBoxRow *row, gpointer user_data)
{
    (void)listbox;
    (void)user_data;
    
    GtkWidget *row_widget = GTK_WIDGET(row);
    GtkWidget *box = gtk_bin_get_child(GTK_BIN(row_widget));
    if (!box) return;
    
    unsigned int *shape_ptr = g_object_get_data(G_OBJECT(box), "cursor-shape");
    if (!shape_ptr) return;
    
    /* Apply the cursor style */
    apply_cursor_style(*shape_ptr);
    
    if (preview_label) {
        const char *cursor_name = "cursor style";
        for (size_t i = 0; i < NUM_STYLES; i++) {
            if (cursor_styles[i].shape == *shape_ptr) {
                cursor_name = cursor_styles[i].name;
                break;
            }
        }
        char *msg = g_strdup_printf("✓ Cursor style changed to: %s", cursor_name);
        gtk_label_set_text(GTK_LABEL(preview_label), msg);
        g_free(msg);
    }
}

/**
 * Update the UI to show current selection
 */
static void update_ui_selection(void)
{
    if (!style_listbox) return;
    
    /* Find and select the row with matching cursor shape */
    GList *children = gtk_container_get_children(GTK_CONTAINER(style_listbox));
    for (GList *iter = children; iter; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        GtkWidget *box = gtk_bin_get_child(GTK_BIN(row));
        if (box) {
            unsigned int *shape_ptr = g_object_get_data(G_OBJECT(box), "cursor-shape");
            if (shape_ptr && *shape_ptr == current_shape) {
                gtk_list_box_select_row(style_listbox, GTK_LIST_BOX_ROW(row));
                break;
            }
        }
    }
    g_list_free(children);
    
    /* Update size slider */
    if (size_scale) {
        g_signal_handlers_block_by_func(size_scale, G_CALLBACK(on_size_changed), NULL);
        gtk_range_set_value(GTK_RANGE(size_scale), cursor_size);
        g_signal_handlers_unblock_by_func(size_scale, G_CALLBACK(on_size_changed), NULL);
    }
    
    /* Update color button */
    if (color_button) {
        GdkRGBA color;
        guint r, g, b;
        sscanf(cursor_color + 1, "%02x%02x%02x", &r, &g, &b);
        color.red = r / 255.0;
        color.green = g / 255.0;
        color.blue = b / 255.0;
        color.alpha = 1.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_button), &color);
    }
}

/**
 * Create a row for a single cursor style.
 */
static GtkWidget *create_style_row(const CursorStyle *style, unsigned int shape)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);

    /* Icon label */
    GtkWidget *icon_label = gtk_label_new(style->icon);
    gtk_widget_set_size_request(icon_label, 40, -1);

    /* Name label */
    GtkWidget *name_label = gtk_label_new(style->name);
    gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);

    gtk_box_pack_start(GTK_BOX(box), icon_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), name_label, TRUE, TRUE, 0);

    /* Store shape id */
    unsigned int *shape_copy = g_new(unsigned int, 1);
    *shape_copy = shape;
    g_object_set_data_full(G_OBJECT(box), "cursor-shape", shape_copy, g_free);

    return box;
}

/**
 * Creates the cursor style selection widget with color and size controls.
 */
GtkWidget *mouse_style_widget_new(void)
{
    /* Load saved settings first */
    load_saved_settings();
    
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 10);

    /* Title */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Mouse Cursor Settings</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(main_vbox), title, FALSE, FALSE, 0);

    /* ---- Cursor Size Section ---- */
    GtkWidget *size_frame = gtk_frame_new("Cursor Size");
    GtkWidget *size_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(size_box), 10);
    gtk_container_add(GTK_CONTAINER(size_frame), size_box);

    /* Size labels */
    GtkWidget *size_labels = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *small_label = gtk_label_new("Small (16)");
    gtk_widget_set_halign(small_label, GTK_ALIGN_START);
    GtkWidget *large_label = gtk_label_new("Large (64)");
    gtk_widget_set_halign(large_label, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(size_labels), small_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(size_labels), large_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(size_box), size_labels, FALSE, FALSE, 0);

    /* Size slider (16-64 pixels) */
    size_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 16, 64, 1);
    gtk_scale_set_draw_value(GTK_SCALE(size_scale), TRUE);
    gtk_range_set_value(GTK_RANGE(size_scale), cursor_size);
    g_signal_connect(size_scale, "value-changed", G_CALLBACK(on_size_changed), NULL);
    gtk_box_pack_start(GTK_BOX(size_box), size_scale, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), size_frame, FALSE, FALSE, 0);

    /* ---- Cursor Color Section ---- */
    GtkWidget *color_frame = gtk_frame_new("Cursor Color");
    GtkWidget *color_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(color_box), 10);
    gtk_container_add(GTK_CONTAINER(color_frame), color_box);

    /* Color picker button */
    GdkRGBA default_color;
    gdk_rgba_parse(&default_color, cursor_color);
    color_button = gtk_color_button_new_with_rgba(&default_color);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(color_button), "Choose Cursor Color");
    g_signal_connect(color_button, "color-set", G_CALLBACK(on_color_set), NULL);
    gtk_box_pack_start(GTK_BOX(color_box), color_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), color_frame, FALSE, FALSE, 0);

    /* ---- Cursor Style Section ---- */
    GtkWidget *style_frame = gtk_frame_new("Cursor Style");
    GtkWidget *style_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(style_vbox), 10);
    gtk_container_add(GTK_CONTAINER(style_frame), style_vbox);

    /* Scrolled list of styles */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 200);

    style_listbox = GTK_LIST_BOX(gtk_list_box_new());
    gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(style_listbox));

    for (size_t i = 0; i < NUM_STYLES; i++) {
        GtkWidget *row_box = create_style_row(&cursor_styles[i], cursor_styles[i].shape);
        GtkWidget *row = gtk_list_box_row_new();
        gtk_container_add(GTK_CONTAINER(row), row_box);
        gtk_list_box_insert(style_listbox, row, -1);
    }

    g_signal_connect(style_listbox, "row-activated", G_CALLBACK(on_style_selected), NULL);

    gtk_box_pack_start(GTK_BOX(style_vbox), scrolled, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), style_frame, TRUE, TRUE, 0);

    /* ---- Preview/Status Label ---- */
    preview_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(preview_label), 0.0);
    gtk_label_set_markup(GTK_LABEL(preview_label), "<small>Select a cursor style to apply</small>");
    gtk_box_pack_start(GTK_BOX(main_vbox), preview_label, FALSE, FALSE, 5);
    
    /* Apply saved settings after UI is built */
    g_idle_add((GSourceFunc)update_ui_selection, NULL);

    return main_vbox;
}