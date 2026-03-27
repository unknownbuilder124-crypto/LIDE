#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>

/**
 * Dialog mode enumeration.
 * Determines whether the file dialog is used for opening or saving.
 */
typedef enum {
    DIALOG_MODE_OPEN,  /* File chooser for opening existing images */
    DIALOG_MODE_SAVE   /* File chooser for saving to new location */
} DialogMode;

/**
 * Main application state structure.
 * Encapsulates all UI components, image data, and editing state.
 */
typedef struct {
    GtkWidget *window;           /* Main application window */
    GtkWidget *image;            /* GtkImage widget for displaying current image */
    GtkWidget *event_box;        /* Event box for capturing mouse interactions */
    GtkWidget *overlay;          /* GtkOverlay for drawing selection rectangle */
    GtkWidget *selection_area;   /* Drawing area for rendering crop selection overlay */
    GtkWidget *statusbar;        /* Status bar showing image info and messages */

    GdkPixbuf *original_pixbuf;  /* Original loaded image (unchanged) for revert operation */
    GdkPixbuf *current_pixbuf;   /* Current image after edits (displayed) */
    char *current_filename;      /* File path of currently open image */
    char *current_folder;        /* Current folder for file dialog navigation */
    gboolean is_modified;        /* TRUE if image has unsaved edits */

    /* Crop selection coordinates (in image pixel coordinates, not display) */
    gboolean selecting;          /* TRUE while user is dragging to select */
    double select_start_x;       /* Selection start X in image pixel coordinates */
    double select_start_y;       /* Selection start Y in image pixel coordinates */
    double select_end_x;         /* Selection end X in image pixel coordinates */
    double select_end_y;         /* Selection end Y in image pixel coordinates */
    gboolean has_selection;      /* TRUE if a valid selection exists */

    double zoom_factor;          /* Zoom factor (1.0 = 100%) - currently unused */
} AppState;

/* Forward declarations */
static void open_image(AppState *state, const char *filename);
static void save_image(AppState *state, const char *filename);
static void update_image(AppState *state);
static void clear_selection(AppState *state);
static void update_statusbar(AppState *state);
static void custom_file_dialog(GtkWidget *parent, DialogMode mode, AppState *state);

/* ----------------------------------------------------------------------
 * Helper: update status bar with image information
 * ---------------------------------------------------------------------- */
/**
 * Updates the status bar with current image filename, dimensions, and modified status.
 *
 * @param state Application state containing image data and UI references.
 */
static void update_statusbar(AppState *state) {
    if (!state->statusbar || !state->current_pixbuf) return;
    
    int width = gdk_pixbuf_get_width(state->current_pixbuf);
    int height = gdk_pixbuf_get_height(state->current_pixbuf);
    
    char *basename = state->current_filename ? g_path_get_basename(state->current_filename) : g_strdup("Untitled");
    char *status_text = g_strdup_printf("%s - %dx%d pixels %s", 
                                        basename, 
                                        width, height,
                                        state->is_modified ? "(modified)" : "");
    
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "image_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status_text);
    
    g_free(basename);
    g_free(status_text);
}

/* ----------------------------------------------------------------------
 * Helper: scale pixbuf for display while preserving aspect ratio
 * ---------------------------------------------------------------------- */
/**
 * Scales a GdkPixbuf to fit within specified dimensions while preserving aspect ratio.
 * If the image is already smaller than the max dimensions, returns a reference to the original.
 *
 * @param src        Source pixbuf to scale.
 * @param max_width  Maximum allowed display width.
 * @param max_height Maximum allowed display height.
 * @return Newly referenced or scaled pixbuf. Caller must unref when done.
 */
static GdkPixbuf *scale_pixbuf_for_display(GdkPixbuf *src, int max_width, int max_height) {
    if (!src) return NULL;
    int w = gdk_pixbuf_get_width(src);
    int h = gdk_pixbuf_get_height(src);
    
    if (w <= max_width && h <= max_height) {
        return g_object_ref(src);
    }
    
    double ratio = (double)w / h;
    double target_ratio = (double)max_width / max_height;
    
    if (ratio > target_ratio) {
        w = max_width;
        h = max_width / ratio;
    } else {
        h = max_height;
        w = max_height * ratio;
    }
    
    return gdk_pixbuf_scale_simple(src, w, h, GDK_INTERP_BILINEAR);
}

/* ----------------------------------------------------------------------
 * Update the GtkImage with current pixbuf, scaled to fit current allocation
 * ---------------------------------------------------------------------- */
/**
 * Updates the displayed image by scaling the current pixbuf to fit the available space.
 * Called after image changes or window resize events.
 *
 * @param state Application state.
 */
static void update_image(AppState *state) {
    if (!state->current_pixbuf) {
        gtk_image_clear(GTK_IMAGE(state->image));
        return;
    }

    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(state->event_box), &alloc);
    if (alloc.width <= 1 || alloc.height <= 1) {
        /* Widget not yet realized - queue for later */
        g_idle_add((GSourceFunc)update_image, state);
        return;
    }

    /* Scale for display */
    GdkPixbuf *display_pixbuf = scale_pixbuf_for_display(state->current_pixbuf, alloc.width, alloc.height);
    if (display_pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(state->image), display_pixbuf);
        g_object_unref(display_pixbuf);
    }
    
    update_statusbar(state);
    
    /* Redraw selection overlay to maintain visual feedback */
    gtk_widget_queue_draw(state->selection_area);
}

/* ----------------------------------------------------------------------
 * Clear selection rectangle
 * ---------------------------------------------------------------------- */
/**
 * Clears the current crop selection and hides the selection overlay.
 *
 * @param state Application state.
 */
static void clear_selection(AppState *state) {
    state->has_selection = FALSE;
    state->selecting = FALSE;
    gtk_widget_queue_draw(state->selection_area);
}

/* ----------------------------------------------------------------------
 * Crop image to selected area
 * ---------------------------------------------------------------------- */
/**
 * Crops the current image to the selected rectangle.
 * Creates a new pixbuf subregion and replaces the current image.
 *
 * @param state Application state.
 *
 * @sideeffect Updates current_pixbuf with cropped version.
 * @sideeffect Sets is_modified to TRUE.
 */
static void crop_image(AppState *state) {
    if (!state->has_selection || !state->current_pixbuf) return;

    /* Ensure coordinates are ordered (x1 <= x2, y1 <= y2) */
    int x1 = (int)state->select_start_x;
    int y1 = (int)state->select_start_y;
    int x2 = (int)state->select_end_x;
    int y2 = (int)state->select_end_y;
    if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
    if (y1 > y2) { int tmp = y1; y1 = y2; y2 = tmp; }

    int width = x2 - x1;
    int height = y2 - y1;
    if (width < 5 || height < 5) {
        /* Selection too small - ignore */
        clear_selection(state);
        return;
    }

    /* Clip to image bounds */
    int img_w = gdk_pixbuf_get_width(state->current_pixbuf);
    int img_h = gdk_pixbuf_get_height(state->current_pixbuf);
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > img_w) x2 = img_w;
    if (y2 > img_h) y2 = img_h;
    width = x2 - x1;
    height = y2 - y1;

    GdkPixbuf *cropped = gdk_pixbuf_new_subpixbuf(state->current_pixbuf, x1, y1, width, height);
    g_object_unref(state->current_pixbuf);
    state->current_pixbuf = g_object_ref(cropped);
    g_object_unref(cropped);
    state->is_modified = TRUE;
    clear_selection(state);
    update_image(state);
}

/* ----------------------------------------------------------------------
 * Rotate image by specified angle
 * ---------------------------------------------------------------------- */
/**
 * Rotates the current image by the specified angle (90, 180, or 270 degrees).
 *
 * @param state Application state.
 * @param angle Rotation angle in degrees (90, 180, 270).
 *
 * @sideeffect Updates current_pixbuf with rotated version.
 * @sideeffect Sets is_modified to TRUE.
 */
static void rotate_image(AppState *state, int angle) {
    if (!state->current_pixbuf) return;
    GdkPixbuf *rotated = gdk_pixbuf_rotate_simple(state->current_pixbuf, angle);
    if (rotated) {
        g_object_unref(state->current_pixbuf);
        state->current_pixbuf = rotated;
        state->is_modified = TRUE;
        update_image(state);
    }
}

/* ----------------------------------------------------------------------
 * Flip image horizontally or vertically
 * ---------------------------------------------------------------------- */
/**
 * Flips the current image horizontally or vertically.
 *
 * @param state      Application state.
 * @param horizontal TRUE for horizontal flip, FALSE for vertical.
 *
 * @sideeffect Updates current_pixbuf with flipped version.
 * @sideeffect Sets is_modified to TRUE.
 */
static void flip_image(AppState *state, gboolean horizontal) {
    if (!state->current_pixbuf) return;
    GdkPixbuf *flipped = gdk_pixbuf_flip(state->current_pixbuf, horizontal);
    if (flipped) {
        g_object_unref(state->current_pixbuf);
        state->current_pixbuf = flipped;
        state->is_modified = TRUE;
        update_image(state);
    }
}

/* ----------------------------------------------------------------------
 * Zoom functions
 * ---------------------------------------------------------------------- */
static void zoom_in(AppState *state) {
    state->zoom_factor *= 1.2;
    update_image(state);
}

static void zoom_out(AppState *state) {
    state->zoom_factor /= 1.2;
    update_image(state);
}

static void zoom_fit(AppState *state) {
    state->zoom_factor = 1.0;
    update_image(state);
}

static void zoom_actual(AppState *state) {
    state->zoom_factor = 1.0;
    update_image(state);
}

/* ----------------------------------------------------------------------
 * Revert to original image (discard all changes)
 * ---------------------------------------------------------------------- */
/**
 * Discards all edits and reverts to the originally loaded image.
 *
 * @param state Application state.
 *
 * @sideeffect Replaces current_pixbuf with original_pixbuf.
 * @sideeffect Clears is_modified flag and selection.
 */
static void revert_to_original(AppState *state) {
    if (state->original_pixbuf) {
        if (state->current_pixbuf) g_object_unref(state->current_pixbuf);
        state->current_pixbuf = g_object_ref(state->original_pixbuf);
        state->is_modified = FALSE;
        clear_selection(state);
        update_image(state);
    }
}

/* ----------------------------------------------------------------------
 * Open image from file
 * ---------------------------------------------------------------------- */
/**
 * Loads an image from the specified file path.
 * Updates all application state and UI accordingly.
 *
 * @param state    Application state.
 * @param filename Path to image file.
 *
 * @sideeffect Loads image into memory, updates window title and status.
 * @sideeffect Shows error dialog if file cannot be loaded.
 */
static void open_image(AppState *state, const char *filename) {
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    if (!pixbuf) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_OK,
                                                    "Failed to load image: %s\n\n"
                                                    "Make sure the file exists and is a valid image format.",
                                                    filename);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    /* Free previous resources */
    if (state->original_pixbuf) g_object_unref(state->original_pixbuf);
    if (state->current_pixbuf) g_object_unref(state->current_pixbuf);
    g_free(state->current_filename);
    g_free(state->current_folder);

    state->original_pixbuf = pixbuf;
    state->current_pixbuf = g_object_ref(pixbuf);
    state->current_filename = g_strdup(filename);
    
    /* Store folder for file chooser default location */
    char *dir = g_path_get_dirname(filename);
    state->current_folder = g_strdup(dir);
    g_free(dir);
    
    state->is_modified = FALSE;
    state->zoom_factor = 1.0;
    clear_selection(state);

    /* Update window title */
    char *basename = g_path_get_basename(filename);
    char *title = g_strdup_printf("BlackLine Image Viewer - %s", basename);
    gtk_window_set_title(GTK_WINDOW(state->window), title);
    g_free(basename);
    g_free(title);

    update_image(state);
}

/* ----------------------------------------------------------------------
 * Save image to file
 * ---------------------------------------------------------------------- */
/**
 * Saves the current image to the specified file path.
 * Determines file format from file extension (defaults to PNG).
 *
 * @param state    Application state.
 * @param filename Destination file path.
 *
 * @sideeffect Writes image data to disk.
 * @sideeffect Updates current_filename and is_modified on success.
 * @sideeffect Shows error dialog on failure.
 */
static void save_image(AppState *state, const char *filename) {
    if (!state->current_pixbuf) return;

    GError *error = NULL;
    
    /* Determine file type from extension */
    const char *ext = strrchr(filename, '.');
    const char *type = "png";
    if (ext) {
        if (g_ascii_strcasecmp(ext, ".jpg") == 0 || g_ascii_strcasecmp(ext, ".jpeg") == 0)
            type = "jpeg";
        else if (g_ascii_strcasecmp(ext, ".bmp") == 0)
            type = "bmp";
        else if (g_ascii_strcasecmp(ext, ".gif") == 0)
            type = "gif";
    }

    gdk_pixbuf_save(state->current_pixbuf, filename, type, &error, NULL);
    if (error) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_OK,
                                                    "Failed to save image: %s", error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return;
    }

    /* Update filename if saved to new location */
    if (g_strcmp0(filename, state->current_filename) != 0) {
        g_free(state->current_filename);
        state->current_filename = g_strdup(filename);
        
        char *dir = g_path_get_dirname(filename);
        g_free(state->current_folder);
        state->current_folder = g_strdup(dir);
        g_free(dir);
    }
    state->is_modified = FALSE;

    /* Update window title */
    char *basename = g_path_get_basename(filename);
    char *title = g_strdup_printf("BlackLine Image Viewer - %s", basename);
    gtk_window_set_title(GTK_WINDOW(state->window), title);
    g_free(basename);
    g_free(title);
    
    update_statusbar(state);
}

/* ----------------------------------------------------------------------
 * Custom file dialog implementation
 * ---------------------------------------------------------------------- */
static char *current_folder_dialog = NULL;

static void on_up_clicked(GtkButton *button, gpointer dialog) {
    (void)button;
    char *parent = g_path_get_dirname(current_folder_dialog);
    if (g_strcmp0(parent, current_folder_dialog) != 0) {
        g_free(current_folder_dialog);
        current_folder_dialog = parent;
        GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
        AppState *state = g_object_get_data(G_OBJECT(parent_win), "app-state");
        gtk_widget_destroy(GTK_WIDGET(dialog));
        custom_file_dialog(GTK_WIDGET(parent_win), DIALOG_MODE_OPEN, state);
    } else {
        g_free(parent);
    }
}

static void on_home_clicked(GtkButton *button, gpointer dialog) {
    (void)button;
    const gchar *home = g_get_home_dir();
    g_free(current_folder_dialog);
    current_folder_dialog = g_strdup(home);
    GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
    AppState *state = g_object_get_data(G_OBJECT(parent_win), "app-state");
    gtk_widget_destroy(GTK_WIDGET(dialog));
    custom_file_dialog(GTK_WIDGET(parent_win), DIALOG_MODE_OPEN, state);
}

static void on_listbox_row_activated(GtkListBox *listbox, GtkListBoxRow *row, gpointer dialog) {
    (void)listbox;
    GtkWidget *row_widget = GTK_WIDGET(row);
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row_widget));
    const char *filename = gtk_label_get_text(GTK_LABEL(label));
    
    if (strncmp(filename, "[DIR] ", 6) == 0) {
        filename = filename + 6;
        char *new_path = g_build_filename(current_folder_dialog, filename, NULL);
        g_free(current_folder_dialog);
        current_folder_dialog = new_path;
        GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
        AppState *state = g_object_get_data(G_OBJECT(parent_win), "app-state");
        gtk_widget_destroy(GTK_WIDGET(dialog));
        custom_file_dialog(GTK_WIDGET(parent_win), DIALOG_MODE_OPEN, state);
    }
}

static void on_open_clicked(GtkButton *button, gpointer dialog) {
    (void)button;
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    char *filename = NULL;
    
    GList *children = gtk_container_get_children(GTK_CONTAINER(content));
    for (GList *c = children; c; c = c->next) {
        if (GTK_IS_BOX(c->data)) {
            GList *box_children = gtk_container_get_children(GTK_CONTAINER(c->data));
            for (GList *bc = box_children; bc; bc = bc->next) {
                if (GTK_IS_BOX(bc->data)) {
                    GList *name_box_children = gtk_container_get_children(GTK_CONTAINER(bc->data));
                    for (GList *nc = name_box_children; nc; nc = nc->next) {
                        if (GTK_IS_ENTRY(nc->data)) {
                            filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(nc->data)));
                        }
                    }
                    g_list_free(name_box_children);
                }
            }
            g_list_free(box_children);
        }
    }
    g_list_free(children);
    
    if (filename && strlen(filename) > 0) {
        char *fullpath = g_build_filename(current_folder_dialog, filename, NULL);
        GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
        AppState *state = g_object_get_data(G_OBJECT(parent_win), "app-state");
        open_image(state, fullpath);
        g_free(fullpath);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void on_save_clicked(GtkButton *button, gpointer dialog) {
    (void)button;
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    char *filename = NULL;
    
    GList *children = gtk_container_get_children(GTK_CONTAINER(content));
    for (GList *c = children; c; c = c->next) {
        if (GTK_IS_BOX(c->data)) {
            GList *box_children = gtk_container_get_children(GTK_CONTAINER(c->data));
            for (GList *bc = box_children; bc; bc = bc->next) {
                if (GTK_IS_BOX(bc->data)) {
                    GList *name_box_children = gtk_container_get_children(GTK_CONTAINER(bc->data));
                    for (GList *nc = name_box_children; nc; nc = nc->next) {
                        if (GTK_IS_ENTRY(nc->data)) {
                            filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(nc->data)));
                        }
                    }
                    g_list_free(name_box_children);
                }
            }
            g_list_free(box_children);
        }
    }
    g_list_free(children);
    
    if (filename && strlen(filename) > 0) {
        char *fullpath = g_build_filename(current_folder_dialog, filename, NULL);
        GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
        AppState *state = g_object_get_data(G_OBJECT(parent_win), "app-state");
        save_image(state, fullpath);
        g_free(fullpath);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    (void)user_data;
    if (response_id == GTK_RESPONSE_DELETE_EVENT) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

static void custom_file_dialog(GtkWidget *parent, DialogMode mode, AppState *state) {
    GtkWidget *dialog;
    GtkWidget *content;
    GtkWidget *vbox;
    GtkWidget *scrolled;
    GtkWidget *listbox;
    GtkWidget *button_box;
    GtkWidget *cancel_btn;
    GtkWidget *action_btn;
    GtkWidget *entry;
    GtkWidget *path_box;
    GtkWidget *up_btn;
    GtkWidget *home_btn;
    GtkWidget *path_label;
    
    const gchar *home = g_get_home_dir();
    if (!current_folder_dialog) {
        current_folder_dialog = g_strdup(state->current_folder ? state->current_folder : home);
    }
    
    dialog = gtk_dialog_new();
    if (mode == DIALOG_MODE_OPEN) {
        gtk_window_set_title(GTK_WINDOW(dialog), "Open Image");
    } else {
        gtk_window_set_title(GTK_WINDOW(dialog), "Save Image");
    }
    
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 400);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);
    
    path_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), path_box, FALSE, FALSE, 0);
    
    up_btn = gtk_button_new_with_label("↑");
    gtk_widget_set_size_request(up_btn, 30, 25);
    g_signal_connect(up_btn, "clicked", G_CALLBACK(on_up_clicked), dialog);
    gtk_box_pack_start(GTK_BOX(path_box), up_btn, FALSE, FALSE, 0);
    
    home_btn = gtk_button_new_with_label("🏠");
    gtk_widget_set_size_request(home_btn, 30, 25);
    g_signal_connect(home_btn, "clicked", G_CALLBACK(on_home_clicked), dialog);
    gtk_box_pack_start(GTK_BOX(path_box), home_btn, FALSE, FALSE, 0);
    
    path_label = gtk_label_new(current_folder_dialog);
    gtk_label_set_ellipsize(GTK_LABEL(path_label), PANGO_ELLIPSIZE_START);
    gtk_box_pack_start(GTK_BOX(path_box), path_label, TRUE, TRUE, 5);
    
    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 250);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    DIR *dir = opendir(current_folder_dialog);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            
            char *fullpath = g_build_filename(current_folder_dialog, entry->d_name, NULL);
            struct stat st;
            if (stat(fullpath, &st) == 0) {
                GtkWidget *row = gtk_list_box_row_new();
                char *display_name;
                if (S_ISDIR(st.st_mode)) {
                    display_name = g_strdup_printf("[DIR] %s", entry->d_name);
                } else {
                    display_name = g_strdup(entry->d_name);
                }
                GtkWidget *label = gtk_label_new(display_name);
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);
                gtk_container_add(GTK_CONTAINER(row), label);
                g_free(display_name);
                gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
            }
            g_free(fullpath);
        }
        closedir(dir);
    }
    
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), name_box, FALSE, FALSE, 0);
    
    GtkWidget *name_label = gtk_label_new("Name:");
    gtk_box_pack_start(GTK_BOX(name_box), name_label, FALSE, FALSE, 0);
    
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(name_box), entry, TRUE, TRUE, 0);
    
    if (mode == DIALOG_MODE_SAVE && state->current_filename) {
        char *basename = g_path_get_basename(state->current_filename);
        gtk_entry_set_text(GTK_ENTRY(entry), basename);
        g_free(basename);
    } else if (mode == DIALOG_MODE_SAVE) {
        gtk_entry_set_text(GTK_ENTRY(entry), "untitled.png");
    }
    
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
    
    cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, TRUE, TRUE, 0);
    
    if (mode == DIALOG_MODE_OPEN) {
        action_btn = gtk_button_new_with_label("Open");
        g_signal_connect(action_btn, "clicked", G_CALLBACK(on_open_clicked), dialog);
    } else {
        action_btn = gtk_button_new_with_label("Save");
        g_signal_connect(action_btn, "clicked", G_CALLBACK(on_save_clicked), dialog);
    }
    gtk_box_pack_start(GTK_BOX(button_box), action_btn, TRUE, TRUE, 0);
    
    g_signal_connect(listbox, "row-activated", G_CALLBACK(on_listbox_row_activated), dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), NULL);
    
    gtk_widget_show_all(dialog);
}

/* ----------------------------------------------------------------------
 * Menu callbacks
 * ---------------------------------------------------------------------- */
static void on_open_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    custom_file_dialog(state->window, DIALOG_MODE_OPEN, state);
}

static void on_save_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    if (state->current_filename) {
        save_image(state, state->current_filename);
    } else {
        custom_file_dialog(state->window, DIALOG_MODE_SAVE, state);
    }
}

static void on_save_as_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    custom_file_dialog(state->window, DIALOG_MODE_SAVE, state);
}

static void on_quit_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    gtk_window_close(GTK_WINDOW(state->window));
}

static void on_crop_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    crop_image(state);
}

static void on_rotate_left_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    rotate_image(state, 270);
}

static void on_rotate_right_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    rotate_image(state, 90);
}

static void on_flip_h_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    flip_image(state, TRUE);
}

static void on_flip_v_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    flip_image(state, FALSE);
}

static void on_zoom_in_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    zoom_in(state);
}

static void on_zoom_out_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    zoom_out(state);
}

static void on_zoom_fit_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    zoom_fit(state);
}

static void on_zoom_actual_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    zoom_actual(state);
}

static void on_undo_activate(GtkMenuItem *item, AppState *state) {
    (void)item;
    revert_to_original(state);
}

/* ----------------------------------------------------------------------
 * Button click handlers
 * ---------------------------------------------------------------------- */
static void on_open_button_clicked(GtkButton *button, AppState *state) {
    (void)button;
    custom_file_dialog(state->window, DIALOG_MODE_OPEN, state);
}

static void on_save_button_clicked(GtkButton *button, AppState *state) {
    (void)button;
    if (state->current_filename && !state->is_modified) {
        save_image(state, state->current_filename);
    } else if (state->current_filename) {
        save_image(state, state->current_filename);
    } else {
        custom_file_dialog(state->window, DIALOG_MODE_SAVE, state);
    }
}

/* ----------------------------------------------------------------------
 * Mouse event handlers for crop selection
 * ---------------------------------------------------------------------- */
/**
 * Callback for mouse button press on the image area.
 * Initiates crop selection when clicking inside the image.
 *
 * @param widget Event box widget.
 * @param event  Button event details.
 * @param state  Application state.
 * @return       TRUE to stop event propagation.
 */
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, AppState *state) {
    if (!state->current_pixbuf) return FALSE;
    if (event->button == 1) {
        double x = event->x;
        double y = event->y;
        
        GtkAllocation alloc;
        gtk_widget_get_allocation(widget, &alloc);
        
        GdkPixbuf *display = gtk_image_get_pixbuf(GTK_IMAGE(state->image));
        if (!display) return FALSE;
        
        int disp_w = gdk_pixbuf_get_width(display);
        int disp_h = gdk_pixbuf_get_height(display);
        int off_x = (alloc.width - disp_w) / 2;
        int off_y = (alloc.height - disp_h) / 2;
        
        if (x < off_x || x >= off_x + disp_w || y < off_y || y >= off_y + disp_h)
            return FALSE;
        
        int img_w = gdk_pixbuf_get_width(state->current_pixbuf);
        int img_h = gdk_pixbuf_get_height(state->current_pixbuf);
        double scale_x = (double)img_w / disp_w;
        double scale_y = (double)img_h / disp_h;
        
        state->select_start_x = (x - off_x) * scale_x;
        state->select_start_y = (y - off_y) * scale_y;
        state->select_end_x = state->select_start_x;
        state->select_end_y = state->select_start_y;
        state->selecting = TRUE;
        state->has_selection = TRUE;
        gtk_widget_queue_draw(state->selection_area);
    }
    return TRUE;
}

/**
 * Callback for mouse button release on the image area.
 * Finalizes crop selection and clears selection if it's too small.
 *
 * @param widget Event box widget.
 * @param event  Button event details.
 * @param state  Application state.
 * @return       TRUE to stop event propagation.
 */
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, AppState *state) {
    (void)widget;
    if (event->button == 1 && state->selecting) {
        state->selecting = FALSE;
        double dx = state->select_end_x - state->select_start_x;
        double dy = state->select_end_y - state->select_start_y;
        if (dx * dx < 4 && dy * dy < 4) {
            clear_selection(state);
        } else {
            gtk_widget_queue_draw(state->selection_area);
        }
    }
    return TRUE;
}

/**
 * Callback for mouse motion on the image area.
 * Updates the selection rectangle while dragging.
 *
 * @param widget Event box widget.
 * @param event  Motion event details.
 * @param state  Application state.
 * @return       TRUE if event was handled.
 */
static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, AppState *state) {
    if (!state->selecting || !state->current_pixbuf) return FALSE;

    double x = event->x;
    double y = event->y;
    
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    
    GdkPixbuf *display = gtk_image_get_pixbuf(GTK_IMAGE(state->image));
    if (!display) return FALSE;
    
    int disp_w = gdk_pixbuf_get_width(display);
    int disp_h = gdk_pixbuf_get_height(display);
    int off_x = (alloc.width - disp_w) / 2;
    int off_y = (alloc.height - disp_h) / 2;
    
    if (x < off_x) x = off_x;
    if (x >= off_x + disp_w) x = off_x + disp_w - 1;
    if (y < off_y) y = off_y;
    if (y >= off_y + disp_h) y = off_y + disp_h - 1;

    int img_w = gdk_pixbuf_get_width(state->current_pixbuf);
    int img_h = gdk_pixbuf_get_height(state->current_pixbuf);
    double scale_x = (double)img_w / disp_w;
    double scale_y = (double)img_h / disp_h;
    
    state->select_end_x = (x - off_x) * scale_x;
    state->select_end_y = (y - off_y) * scale_y;
    gtk_widget_queue_draw(state->selection_area);
    return TRUE;
}

/**
 * Callback for drawing the selection rectangle on the overlay.
 *
 * @param widget Drawing area widget.
 * @param cr     Cairo context for drawing.
 * @param state  Application state.
 * @return       FALSE to allow further drawing (unused).
 */
static gboolean on_draw_selection(GtkWidget *widget, cairo_t *cr, AppState *state) {
    (void)widget;
    if (!state->has_selection || !state->current_pixbuf) return FALSE;

    GtkWidget *event_box = state->event_box;
    GtkAllocation alloc;
    gtk_widget_get_allocation(event_box, &alloc);
    
    GdkPixbuf *display = gtk_image_get_pixbuf(GTK_IMAGE(state->image));
    if (!display) return FALSE;
    
    int disp_w = gdk_pixbuf_get_width(display);
    int disp_h = gdk_pixbuf_get_height(display);
    int off_x = (alloc.width - disp_w) / 2;
    int off_y = (alloc.height - disp_h) / 2;

    int img_w = gdk_pixbuf_get_width(state->current_pixbuf);
    int img_h = gdk_pixbuf_get_height(state->current_pixbuf);
    double scale_x = (double)disp_w / img_w;
    double scale_y = (double)disp_h / img_h;

    double x1 = off_x + state->select_start_x * scale_x;
    double y1 = off_y + state->select_start_y * scale_y;
    double x2 = off_x + state->select_end_x * scale_x;
    double y2 = off_y + state->select_end_y * scale_y;

    if (x1 > x2) { double tmp = x1; x1 = x2; x2 = tmp; }
    if (y1 > y2) { double tmp = y1; y1 = y2; y2 = tmp; }

    cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.8);
    cairo_set_line_width(cr, 2.0);
    double dashes[] = {4.0, 4.0};
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
    cairo_stroke(cr);

    return FALSE;
}

/**
 * Callback for window delete event.
 * Prompts to save changes if the image has been modified.
 *
 * @param widget Window widget.
 * @param event  Event details (unused).
 * @param state  Application state.
 * @return       FALSE to allow default handling.
 */
static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, AppState *state) {
    (void)widget;
    (void)event;
    
    if (state->is_modified) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_QUESTION,
                                                    GTK_BUTTONS_YES_NO,
                                                    "Save changes before closing?");
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_YES) {
            if (state->current_filename) {
                save_image(state, state->current_filename);
            } else {
                custom_file_dialog(state->window, DIALOG_MODE_SAVE, state);
            }
        }
    }
    
    return FALSE;
}

/* ----------------------------------------------------------------------
 * Main window creation
 * ---------------------------------------------------------------------- */
/**
 * Creates and displays the main image viewer window.
 *
 * @param app        GtkApplication instance.
 * @param user_data  User data (unused).
 *
 * @sideeffect Initializes all UI components and sets up signal handlers.
 */

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    AppState *state = g_new0(AppState, 1);
    state->zoom_factor = 1.0;
    state->window = gtk_application_window_new(app);
    
    /* Get monitor geometry for positioning below panel */
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    if (!monitor) monitor = gdk_display_get_monitor_at_point(display, 0, 0);
    GdkRectangle monitor_geom;
    gdk_monitor_get_geometry(monitor, &monitor_geom);
    
    /* Panel height (adjust based on your panel) */
    int panel_height = 40;
    
    /* Position window below panel */
    int window_width = 800;
    int window_height = 600;
    
    int pos_x = (monitor_geom.width - window_width) / 2; /* Center horizontally */
    int pos_y = panel_height + 10; /* Panel height + small gap */
    
    /* Ensure window stays within screen bounds */
    if (pos_x < 0) pos_x = 10;
    if (pos_y + window_height > monitor_geom.height) {
        pos_y = monitor_geom.height - window_height - 10;
    }
    
    gtk_window_move(GTK_WINDOW(state->window), pos_x, pos_y);
    gtk_window_set_title(GTK_WINDOW(state->window), "BlackLine Image Viewer");
    gtk_window_set_default_size(GTK_WINDOW(state->window), window_width, window_height);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_NONE);
    gtk_window_set_icon_name(GTK_WINDOW(state->window), "image-x-generic");

    /* Header bar with toolbar buttons */
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Image Viewer");
    gtk_window_set_titlebar(GTK_WINDOW(state->window), header);

    /* Toolbar buttons */
    GtkWidget *open_btn = gtk_button_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_open_button_clicked), state);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), open_btn);

    GtkWidget *save_btn = gtk_button_new_from_icon_name("document-save", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_button_clicked), state);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), save_btn);

    GtkWidget *crop_btn = gtk_button_new_with_label("Crop");
    g_signal_connect(crop_btn, "clicked", G_CALLBACK(on_crop_activate), state);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), crop_btn);

    GtkWidget *rotate_left_btn = gtk_button_new_from_icon_name("object-rotate-left", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(rotate_left_btn, "clicked", G_CALLBACK(on_rotate_left_activate), state);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), rotate_left_btn);

    GtkWidget *rotate_right_btn = gtk_button_new_from_icon_name("object-rotate-right", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(rotate_right_btn, "clicked", G_CALLBACK(on_rotate_right_activate), state);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), rotate_right_btn);

    /* Menubar */
    GtkWidget *menubar = gtk_menu_bar_new();
    
    /* File menu */
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    
    GtkWidget *open_item = gtk_menu_item_new_with_label("Open");
    g_signal_connect(open_item, "activate", G_CALLBACK(on_open_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    
    GtkWidget *save_item = gtk_menu_item_new_with_label("Save");
    g_signal_connect(save_item, "activate", G_CALLBACK(on_save_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
    
    GtkWidget *save_as_item = gtk_menu_item_new_with_label("Save As...");
    g_signal_connect(save_as_item, "activate", G_CALLBACK(on_save_as_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_as_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);
    
    /* Edit menu */
    GtkWidget *edit_menu = gtk_menu_new();
    GtkWidget *edit_item = gtk_menu_item_new_with_label("Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);
    
    GtkWidget *crop_item = gtk_menu_item_new_with_label("Crop");
    g_signal_connect(crop_item, "activate", G_CALLBACK(on_crop_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), crop_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    
    GtkWidget *rotate_left_item = gtk_menu_item_new_with_label("Rotate Left");
    g_signal_connect(rotate_left_item, "activate", G_CALLBACK(on_rotate_left_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), rotate_left_item);
    
    GtkWidget *rotate_right_item = gtk_menu_item_new_with_label("Rotate Right");
    g_signal_connect(rotate_right_item, "activate", G_CALLBACK(on_rotate_right_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), rotate_right_item);
    
    GtkWidget *flip_h_item = gtk_menu_item_new_with_label("Flip Horizontal");
    g_signal_connect(flip_h_item, "activate", G_CALLBACK(on_flip_h_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), flip_h_item);
    
    GtkWidget *flip_v_item = gtk_menu_item_new_with_label("Flip Vertical");
    g_signal_connect(flip_v_item, "activate", G_CALLBACK(on_flip_v_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), flip_v_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    
    GtkWidget *undo_item = gtk_menu_item_new_with_label("Revert to Original");
    g_signal_connect(undo_item, "activate", G_CALLBACK(on_undo_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), undo_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit_item);
    
    /* View menu */
    GtkWidget *view_menu = gtk_menu_new();
    GtkWidget *view_item = gtk_menu_item_new_with_label("View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);
    
    GtkWidget *zoom_in_item = gtk_menu_item_new_with_label("Zoom In");
    g_signal_connect(zoom_in_item, "activate", G_CALLBACK(on_zoom_in_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), zoom_in_item);
    
    GtkWidget *zoom_out_item = gtk_menu_item_new_with_label("Zoom Out");
    g_signal_connect(zoom_out_item, "activate", G_CALLBACK(on_zoom_out_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), zoom_out_item);
    
    GtkWidget *zoom_fit_item = gtk_menu_item_new_with_label("Fit to Window");
    g_signal_connect(zoom_fit_item, "activate", G_CALLBACK(on_zoom_fit_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), zoom_fit_item);
    
    GtkWidget *zoom_actual_item = gtk_menu_item_new_with_label("Actual Size");
    g_signal_connect(zoom_actual_item, "activate", G_CALLBACK(on_zoom_actual_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), zoom_actual_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view_item);

    /* Main layout */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    /* Overlay for image and selection rectangle */
    state->overlay = gtk_overlay_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->overlay, TRUE, TRUE, 0);

    /* Event box for image mouse events */
    state->event_box = gtk_event_box_new();
    gtk_widget_set_events(state->event_box, GDK_BUTTON_PRESS_MASK |
                                           GDK_BUTTON_RELEASE_MASK |
                                           GDK_POINTER_MOTION_MASK);
    gtk_overlay_add_overlay(GTK_OVERLAY(state->overlay), state->event_box);
    g_signal_connect(state->event_box, "button-press-event", G_CALLBACK(on_button_press), state);
    g_signal_connect(state->event_box, "button-release-event", G_CALLBACK(on_button_release), state);
    g_signal_connect(state->event_box, "motion-notify-event", G_CALLBACK(on_motion_notify), state);

    state->image = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(state->event_box), state->image);

    /* Selection drawing area overlay */
    state->selection_area = gtk_drawing_area_new();
    gtk_overlay_add_overlay(GTK_OVERLAY(state->overlay), state->selection_area);
    g_signal_connect(state->selection_area, "draw", G_CALLBACK(on_draw_selection), state);
    gtk_widget_set_events(state->selection_area, 0);

    /* Status bar */
    state->statusbar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->statusbar, FALSE, FALSE, 0);

    /* Connect delete event */
    g_signal_connect(state->window, "delete-event", G_CALLBACK(on_delete_event), state);

    /* Apply dark theme CSS */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }"
        "button { background-color: #1e2429; color: #00ff88; border: 1px solid #00ff88; }"
        "button:hover { background-color: #2a323a; }"
        "label { color: #ffffff; }"
        "menubar { background-color: #0b0f14; color: #ffffff; }"
        "menuitem { background-color: #0b0f14; color: #ffffff; }"
        "menuitem:hover { background-color: #2a323a; }"
        "statusbar { background-color: #0b0f14; color: #00ff88; }"
        "dialog { background-color: #0b0f14; color: #ffffff; }"
        "list { background-color: #1a1a1a; color: #ffffff; }"
        "listboxrow { background-color: #1a1a1a; color: #ffffff; }"
        "listboxrow:hover { background-color: #333333; }"
        "entry { background-color: #000000; color: #ffffff; border: 1px solid #00ff88; }",
        -1, NULL);
        
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    gtk_widget_show_all(state->window);

    /* Show welcome message */
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "image_info");
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, 
                      "Open an image using File → Open or the Open button");

    g_object_set_data_full(G_OBJECT(state->window), "app-state", state, g_free);
}

/**
 * Open signal handler for when files are passed as command-line arguments.
 *
 * @param app   GtkApplication instance.
 * @param files Array of GFile objects passed to the application.
 * @param n_files Number of files.
 * @param hint  Unused hint parameter.
 * @param user_data User data (unused).
 */
static void on_open(GtkApplication *app, GFile **files, gint n_files, const gchar *hint, gpointer user_data) {
    (void)hint;
    (void)user_data;
    
    if (n_files <= 0) return;
    
    GtkWindow *window = gtk_application_get_active_window(app);
    if (!window) {
        g_signal_emit_by_name(app, "activate");
        window = gtk_application_get_active_window(app);
    }
    
    if (!window) return;
    
    AppState *state = (AppState *)g_object_get_data(G_OBJECT(window), "app-state");
    if (!state) return;
    
    gchar *path = g_file_get_path(files[0]);
    if (path) {
        open_image(state, path);
        g_free(path);
    }
}

/**
 * Activate signal handler for when app starts with no files.
 *
 * @param app        GtkApplication instance.
 * @param user_data  User data (unused).
 */
static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    activate(app, NULL);
}

/* ----------------------------------------------------------------------
 * Application entry point
 * ---------------------------------------------------------------------- */
int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.blackline.imageviewer", G_APPLICATION_HANDLES_OPEN);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    g_signal_connect(app, "open", G_CALLBACK(on_open), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}