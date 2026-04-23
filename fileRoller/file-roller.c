#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include "file-roller.h"
#include "../controls/optionals/FileChooser.h"

/**
 * file-roller.c - Multi-tab Universal File Viewer/Editor
 *
 * Universal file viewer/editor for LIDE desktop environment with tab support.
 * Supports images, text, PDFs, videos, archives, and more.
 * Text files can be edited and saved.
 * Uses custom FileChooser dialog for all file operations.
 *
 * This module is part of the LIDE desktop environment system.
 */

/* ========== FILE TYPE DETECTION ========== */

FileType get_file_type(const char *filename)
{
    if (!filename) return FILE_TYPE_UNKNOWN;

    const char *ext = strrchr(filename, '.');
    if (!ext) return FILE_TYPE_UNKNOWN;

    ext++;
    char ext_lower[256];
    strncpy(ext_lower, ext, sizeof(ext_lower) - 1);
    ext_lower[sizeof(ext_lower) - 1] = '\0';

    for (int i = 0; ext_lower[i]; i++) {
        ext_lower[i] = tolower((unsigned char)ext_lower[i]);
    }

    if (strcmp(ext_lower, "png") == 0 || strcmp(ext_lower, "jpg") == 0 ||
        strcmp(ext_lower, "jpeg") == 0 || strcmp(ext_lower, "gif") == 0 ||
        strcmp(ext_lower, "bmp") == 0 || strcmp(ext_lower, "webp") == 0 ||
        strcmp(ext_lower, "tiff") == 0 || strcmp(ext_lower, "svg") == 0) {
        return FILE_TYPE_IMAGE;
    }

    if (strcmp(ext_lower, "txt") == 0 || strcmp(ext_lower, "c") == 0 ||
        strcmp(ext_lower, "h") == 0 || strcmp(ext_lower, "py") == 0 ||
        strcmp(ext_lower, "js") == 0 || strcmp(ext_lower, "html") == 0 ||
        strcmp(ext_lower, "css") == 0 || strcmp(ext_lower, "json") == 0 ||
        strcmp(ext_lower, "xml") == 0 || strcmp(ext_lower, "sh") == 0 ||
        strcmp(ext_lower, "conf") == 0 || strcmp(ext_lower, "cfg") == 0 ||
        strcmp(ext_lower, "md") == 0 || strcmp(ext_lower, "log") == 0) {
        return FILE_TYPE_TEXT;
    }

    if (strcmp(ext_lower, "pdf") == 0) {
        return FILE_TYPE_PDF;
    }

    if (strcmp(ext_lower, "mp4") == 0 || strcmp(ext_lower, "webm") == 0 ||
        strcmp(ext_lower, "mkv") == 0 || strcmp(ext_lower, "avi") == 0 ||
        strcmp(ext_lower, "mov") == 0 || strcmp(ext_lower, "flv") == 0 ||
        strcmp(ext_lower, "wmv") == 0 || strcmp(ext_lower, "m4v") == 0) {
        return FILE_TYPE_VIDEO;
    }

    if (strcmp(ext_lower, "mp3") == 0 || strcmp(ext_lower, "wav") == 0 ||
        strcmp(ext_lower, "flac") == 0 || strcmp(ext_lower, "aac") == 0 ||
        strcmp(ext_lower, "ogg") == 0 || strcmp(ext_lower, "m4a") == 0 ||
        strcmp(ext_lower, "wma") == 0) {
        return FILE_TYPE_AUDIO;
    }

    if (strcmp(ext_lower, "zip") == 0 || strcmp(ext_lower, "tar") == 0 ||
        strcmp(ext_lower, "gz") == 0 || strcmp(ext_lower, "7z") == 0 ||
        strcmp(ext_lower, "rar") == 0 || strcmp(ext_lower, "xz") == 0 ||
        strcmp(ext_lower, "bz2") == 0) {
        return FILE_TYPE_ARCHIVE;
    }

    return FILE_TYPE_UNKNOWN;
}

const char *get_file_type_name(FileType type)
{
    switch (type) {
        case FILE_TYPE_IMAGE:
            return "Image";
        case FILE_TYPE_TEXT:
            return "Text Document";
        case FILE_TYPE_PDF:
            return "PDF Document";
        case FILE_TYPE_VIDEO:
            return "Video";
        case FILE_TYPE_AUDIO:
            return "Audio";
        case FILE_TYPE_ARCHIVE:
            return "Archive";
        default:
            return "Unknown";
    }
}

gboolean is_file_type_supported(FileType type)
{
    return (type != FILE_TYPE_UNKNOWN);
}

/* ========== TAB STATE STRUCTURE ========== */

typedef struct {
    /* File properties */
    char *filename;                 /* Currently opened file path */
    char *folder;                   /* Current folder for dialogs */
    FileType file_type;             /* Type of current file */
    gboolean is_modified;           /* Whether file has unsaved changes */

    /* Image viewer widgets */
    GtkWidget *image_viewer;        /* Image display area */
    GtkWidget *image_scroll;        /* Scroll container for image */
    GdkPixbuf *current_pixbuf;      /* Current image pixbuf */
    GdkPixbuf *original_pixbuf;     /* Original image pixbuf */
    double zoom_level;              /* Zoom level (1.0 = 100%) */

    /* Text viewer/editor widgets */
    GtkWidget *text_view;           /* Text display/editing area */
    GtkWidget *text_scroll;         /* Scroll container for text */
    gboolean text_modified;         /* Whether text has unsaved changes */

    /* PDF viewer stub */
    GtkWidget *pdf_label;           /* Placeholder for PDF */

    /* Video viewer stub */
    GtkWidget *video_label;         /* Placeholder for video */

    /* Archive browser stub */
    GtkWidget *archive_tree;        /* Archive contents tree */
    GtkWidget *archive_scroll;      /* Scroll container */

    /* Stack for this tab */
    GtkWidget *stack;               /* View stack for different file types */

} TabState;

/* ========== APPLICATION STATE ========== */

typedef struct {
    GtkWidget *window;              /* Main window */
    GtkWidget *notebook;            /* Notebook for tabs */
    GtkWidget *statusbar;           /* Status bar */
    GtkWidget *header_bar;          /* Header bar for title */

    /* Tab management */
    GList *tabs;                    /* List of TabState pointers */
    TabState *current_tab;          /* Currently active tab */
    int tab_count;                  /* Number of open tabs */

    /* Window dragging */
    gboolean is_dragging;
    int drag_start_x, drag_start_y;
    int window_start_x, window_start_y;
    gboolean is_resizing;
    int resize_edge;

} AppState;

/* ========== FORWARD DECLARATIONS ========== */
static void open_file(AppState *state, const char *filename);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, AppState *state);
static void save_file(AppState *state);
static void save_file_as(AppState *state);
static void update_window_title(AppState *state);
static TabState *create_new_tab(AppState *state);
static void close_current_tab(AppState *state);
static void switch_to_tab(AppState *state, TabState *tab);
static void on_text_buffer_changed(GtkTextBuffer *buffer, gpointer user_data);

/* ========== TEXT BUFFER CALLBACK ========== */

static void on_text_buffer_changed(GtkTextBuffer *buffer, gpointer user_data)
{
    AppState *state = (AppState *)user_data;
    (void)buffer;

    if (state->current_tab && !state->current_tab->text_modified) {
        state->current_tab->text_modified = TRUE;
        state->current_tab->is_modified = TRUE;
        update_window_title(state);
    }
}

/* ========== TAB MANAGEMENT FUNCTIONS ========== */

/**
 * Create a new empty tab
 */
static TabState *create_new_tab(AppState *state)
{
    TabState *tab = g_new0(TabState, 1);
    tab->zoom_level = 1.0;
    tab->folder = g_strdup(g_get_home_dir());
    tab->text_modified = FALSE;
    tab->is_modified = FALSE;
    tab->file_type = FILE_TYPE_UNKNOWN;

    /* Create stack for this tab */
    tab->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(tab->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(tab->stack), 200);

    /* Welcome page */
    GtkWidget *welcome_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *welcome_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(welcome_scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *welcome_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(welcome_label),
        "<b><span size='x-large' foreground='#00ff88'>Welcome to File Roller 📁</span></b>\n\n"
        "<b>Universal File Viewer & Editor</b>\n"
        "Open and view any file type with ease\n\n"
        "<b>Getting Started:</b>\n"
        "• <b>Open</b> - Click to open a file (opens in new tab)\n"
        "• <b>Save</b> - Save changes to the current file\n"
        "• <b>Save As</b> - Save with a new name\n\n"
        "<b>Supported File Types:</b>\n"
        "• <b>Images:</b> PNG, JPG, GIF, BMP, WebP, SVG\n"
        "• <b>Text:</b> TXT, Code (C, Python, JS, HTML, CSS, JSON, etc.)\n"
        "• <b>Documents:</b> PDF, Archives (ZIP, TAR, 7Z, etc.)\n"
        "• <b>Media:</b> Videos (MP4, WebM, MKV, AVI) and Audio files\n\n"
        "<b>Image Editing:</b>\n"
        "• <b>🔍+</b> / <b>🔍-</b> - Zoom in/out (Ctrl+Plus/Minus)\n"
        "• <b>Fit</b> - Fit image to window (Ctrl+0)\n\n"
        "<b>Text Editing:</b>\n"
        "• Edit text files directly with syntax highlighting\n"
        "• <b>Copy</b> - Copy entire file content\n"
        "• <b>Ctrl+S</b> - Quick save\n\n"
        "<b>Multi-Tab Support:</b>\n"
        "• Open multiple files at once - each in its own tab\n"
        "• Quick switch between files by clicking tabs\n"
        "• <b>Ctrl+W</b> - Close current tab\n\n"
        "<b>Tips:</b>\n"
        "• Drag & drop files to open them\n"
        "• <b>ℹ️ Props</b> - View file properties\n"
        "• Each tab works independently\n\n"
        "<i>Ready to get started? Click the Open button and select a file!</i>");
    gtk_label_set_line_wrap(GTK_LABEL(welcome_label), TRUE);
    gtk_label_set_selectable(GTK_LABEL(welcome_label), TRUE);
    gtk_widget_set_margin_top(welcome_label, 20);
    gtk_widget_set_margin_bottom(welcome_label, 20);
    gtk_widget_set_margin_start(welcome_label, 30);
    gtk_widget_set_margin_end(welcome_label, 30);
    gtk_container_add(GTK_CONTAINER(welcome_scroll), welcome_label);
    gtk_box_pack_start(GTK_BOX(welcome_page), welcome_scroll, TRUE, TRUE, 0);
    gtk_stack_add_titled(GTK_STACK(tab->stack), welcome_page, "welcome", "Welcome");

    /* Add pages to stack */
    /* Image page */
    GtkWidget *image_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    tab->image_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->image_scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    tab->image_viewer = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(tab->image_scroll), tab->image_viewer);
    gtk_box_pack_start(GTK_BOX(image_page), tab->image_scroll, TRUE, TRUE, 0);
    gtk_stack_add_titled(GTK_STACK(tab->stack), image_page, "image", "Image");

    /* Text page */
    GtkWidget *text_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    tab->text_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->text_scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    tab->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->text_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->text_view), GTK_WRAP_WORD);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view));
    g_signal_connect(buffer, "changed", G_CALLBACK(on_text_buffer_changed), state);

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
                                    "textview { font-family: Monospace; font-size: 11pt; }",
                                    -1, NULL);
    GtkStyleContext *style_context = gtk_widget_get_style_context(tab->text_view);
    gtk_style_context_add_provider(style_context,
                                   GTK_STYLE_PROVIDER(css_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    gtk_container_add(GTK_CONTAINER(tab->text_scroll), tab->text_view);
    gtk_box_pack_start(GTK_BOX(text_page), tab->text_scroll, TRUE, TRUE, 0);
    gtk_stack_add_titled(GTK_STACK(tab->stack), text_page, "text", "Text");

    /* PDF page */
    GtkWidget *pdf_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    tab->pdf_label = gtk_label_new("PDF Viewer\n\nSelect a PDF file to view");
    gtk_label_set_selectable(GTK_LABEL(tab->pdf_label), TRUE);
    gtk_box_pack_start(GTK_BOX(pdf_page), tab->pdf_label, TRUE, TRUE, 0);
    gtk_stack_add_titled(GTK_STACK(tab->stack), pdf_page, "pdf", "PDF");

    /* Video page */
    GtkWidget *video_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    tab->video_label = gtk_label_new("Video Player\n\nSelect a video file to view");
    gtk_label_set_selectable(GTK_LABEL(tab->video_label), TRUE);
    gtk_box_pack_start(GTK_BOX(video_page), tab->video_label, TRUE, TRUE, 0);
    gtk_stack_add_titled(GTK_STACK(tab->stack), video_page, "video", "Video");

    /* Archive page */
    GtkWidget *archive_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    tab->archive_tree = gtk_label_new("Archive Viewer\n\nSelect an archive file to view");
    gtk_label_set_selectable(GTK_LABEL(tab->archive_tree), TRUE);
    gtk_box_pack_start(GTK_BOX(archive_page), tab->archive_tree, TRUE, TRUE, 0);
    gtk_stack_add_titled(GTK_STACK(tab->stack), archive_page, "archive", "Archive");

    gtk_widget_show_all(tab->stack);

    return tab;
}

/**
 * Switch to a specific tab
 */
static void switch_to_tab(AppState *state, TabState *tab)
{
    if (!tab || tab == state->current_tab) return;

    state->current_tab = tab;

    /* Find the page number for this tab */
    gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(state->notebook), tab->stack);
    if (page_num >= 0) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), page_num);
    }

    update_window_title(state);
}

/**
 * Close and free a tab
 */
static void close_tab_data(TabState *tab)
{
    if (!tab) return;

    if (tab->filename) g_free(tab->filename);
    if (tab->folder) g_free(tab->folder);
    if (tab->current_pixbuf) g_object_unref(tab->current_pixbuf);
    if (tab->original_pixbuf) g_object_unref(tab->original_pixbuf);
    if (tab->stack) gtk_widget_destroy(tab->stack);

    g_free(tab);
}

/**
 * Close the current tab and switch to another
 */
static void close_current_tab(AppState *state)
{
    if (!state->current_tab || g_list_length(state->tabs) <= 1) return;

    TabState *tab_to_close = state->current_tab;
    GList *node = g_list_find(state->tabs, tab_to_close);

    /* Find next tab to switch to */
    TabState *next_tab = NULL;
    if (node->next) {
        next_tab = (TabState *)node->next->data;
    } else if (node->prev) {
        next_tab = (TabState *)node->prev->data;
    }

    state->tabs = g_list_remove(state->tabs, tab_to_close);
    state->tab_count--;

    if (next_tab) {
        switch_to_tab(state, next_tab);
    }

    close_tab_data(tab_to_close);

    /* Update notebook */
    gtk_notebook_remove_page(GTK_NOTEBOOK(state->notebook),
                            gtk_notebook_get_current_page(GTK_NOTEBOOK(state->notebook)));
}


static void on_minimize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

static void on_maximize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    if (gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window))) & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_window_unmaximize(GTK_WINDOW(window));
    } else {
        gtk_window_maximize(GTK_WINDOW(window));
    }
}

static void on_close_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)
{
    GtkButton *max_btn = GTK_BUTTON(data);

    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }

    return FALSE;
}

/* ========== WINDOW DRAGGING ========== */

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    AppState *state = (AppState *)data;

    if (event->button == 1) {
        state->is_dragging = TRUE;
        state->drag_start_x = event->x_root;
        state->drag_start_y = event->y_root;
        gtk_window_get_position(GTK_WINDOW(state->window), &state->window_start_x, &state->window_start_y);
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    AppState *state = (AppState *)data;

    if (event->button == 1) {
        state->is_dragging = FALSE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    AppState *state = (AppState *)data;

    if (state->is_dragging) {
        int dx = event->x_root - state->drag_start_x;
        int dy = event->y_root - state->drag_start_y;
        gtk_window_move(GTK_WINDOW(state->window),
                       state->window_start_x + dx,
                       state->window_start_y + dy);
        return TRUE;
    }
    return FALSE;
}

/* ========== IMAGE VIEWER FUNCTIONS ========== */

static GdkPixbuf *scale_pixbuf(GdkPixbuf *pixbuf, int max_width, int max_height)
{
    if (!pixbuf) return NULL;

    int orig_width = gdk_pixbuf_get_width(pixbuf);
    int orig_height = gdk_pixbuf_get_height(pixbuf);

    double scale_x = (double)max_width / orig_width;
    double scale_y = (double)max_height / orig_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;

    if (scale >= 1.0) {
        return g_object_ref(pixbuf);
    }

    int new_width = (int)(orig_width * scale);
    int new_height = (int)(orig_height * scale);

    return gdk_pixbuf_scale_simple(pixbuf, new_width, new_height, GDK_INTERP_BILINEAR);
}

static void load_image(TabState *tab, const char *filename)
{
    if (!filename || !tab) return;

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);

    if (error) {
        g_error_free(error);
        gtk_label_set_text(GTK_LABEL(tab->image_viewer), "Failed to load image");
        return;
    }

    if (!pixbuf) {
        gtk_label_set_text(GTK_LABEL(tab->image_viewer), "Could not open image");
        return;
    }

    if (tab->original_pixbuf) g_object_unref(tab->original_pixbuf);
    if (tab->current_pixbuf) g_object_unref(tab->current_pixbuf);

    tab->original_pixbuf = pixbuf;
    tab->zoom_level = 1.0;

    GdkPixbuf *scaled = scale_pixbuf(pixbuf, 600, 400);
    tab->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(tab->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);

    char status[512];
    snprintf(status, sizeof(status), "%s - %dx%d pixels | Zoom: 100%%",
             g_path_get_basename(filename),
             gdk_pixbuf_get_width(pixbuf),
             gdk_pixbuf_get_height(pixbuf));
}

static void zoom_in(TabState *tab)
{
    if (!tab || !tab->original_pixbuf) return;

    tab->zoom_level *= 1.2;
    if (tab->zoom_level > 5.0) tab->zoom_level = 5.0;

    int new_width = (int)(gdk_pixbuf_get_width(tab->original_pixbuf) * tab->zoom_level);
    int new_height = (int)(gdk_pixbuf_get_height(tab->original_pixbuf) * tab->zoom_level);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(tab->original_pixbuf, new_width, new_height,
                                                 GDK_INTERP_BILINEAR);
    if (!scaled) return;

    if (tab->current_pixbuf) g_object_unref(tab->current_pixbuf);
    tab->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(tab->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);
}

static void zoom_out(TabState *tab)
{
    if (!tab || !tab->original_pixbuf) return;

    tab->zoom_level /= 1.2;
    if (tab->zoom_level < 0.1) tab->zoom_level = 0.1;

    int new_width = (int)(gdk_pixbuf_get_width(tab->original_pixbuf) * tab->zoom_level);
    int new_height = (int)(gdk_pixbuf_get_height(tab->original_pixbuf) * tab->zoom_level);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(tab->original_pixbuf, new_width, new_height,
                                                 GDK_INTERP_BILINEAR);
    if (!scaled) return;

    if (tab->current_pixbuf) g_object_unref(tab->current_pixbuf);
    tab->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(tab->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);
}

static void fit_image_to_window(TabState *tab)
{
    if (!tab || !tab->original_pixbuf) return;

    tab->zoom_level = 1.0;
    GdkPixbuf *scaled = scale_pixbuf(tab->original_pixbuf, 600, 400);

    if (tab->current_pixbuf) g_object_unref(tab->current_pixbuf);
    tab->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(tab->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);
}

/* ========== TEXT EDITOR FUNCTIONS ========== */

static void load_text_file(TabState *tab, const char *filename)
{
    if (!filename || !tab) return;

    GError *error = NULL;
    gchar *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(filename, &contents, &length, &error)) {
        gchar error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Error loading file: %s",
                 error ? error->message : "Unknown error");
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view)),
                                 error_msg, -1);
        if (error) g_error_free(error);
        return;
    }

    gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->text_view), TRUE);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view)),
                             contents ? contents : "", -1);

    tab->text_modified = FALSE;
    tab->is_modified = FALSE;

    g_free(contents);
}

static void save_file(AppState *state)
{
    TabState *tab = state->current_tab;
    if (!tab || !tab->filename) {
        save_file_as(state);
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    GError *error = NULL;
    if (!g_file_set_contents(tab->filename, text, -1, &error)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Failed to save file: %s",
                                                   error ? error->message : "Unknown error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (error) g_error_free(error);
    } else {
        tab->text_modified = FALSE;
        tab->is_modified = FALSE;
        update_window_title(state);
    }

    g_free(text);
}

static void save_file_as(AppState *state)
{
    TabState *tab = state->current_tab;
    if (!tab) return;

    const char *initial_dir = tab->folder ? tab->folder : g_get_home_dir();

    FileChooser *chooser = file_chooser_new(CHOOSER_FILE, CHOOSER_ACTION_SAVE, initial_dir);
    if (!chooser) return;

    if (tab->filename) {
        const char *basename = g_path_get_basename(tab->filename);
        gtk_entry_set_text(GTK_ENTRY(chooser->filename_entry), basename);
        g_free((void*)basename);
    } else {
        gtk_entry_set_text(GTK_ENTRY(chooser->filename_entry), "untitled.txt");
    }

    char *selected_path = file_chooser_show(chooser);

    if (selected_path) {
        if (tab->folder) g_free(tab->folder);
        tab->folder = g_path_get_dirname(selected_path);

        if (tab->filename) g_free(tab->filename);
        tab->filename = g_strdup(selected_path);

        save_file(state);
        update_window_title(state);

        g_free(selected_path);
    }

    file_chooser_destroy(chooser);
}

static void open_file_dialog(AppState *state)
{
    if (!state->current_tab) return;

    const char *initial_dir = state->current_tab->folder ? state->current_tab->folder : g_get_home_dir();

    FileChooser *chooser = file_chooser_new(CHOOSER_FILE, CHOOSER_ACTION_OPEN, initial_dir);
    if (!chooser) return;

    char *selected_path = file_chooser_show(chooser);

    if (selected_path) {
        open_file(state, selected_path);
        g_free(selected_path);
    }

    file_chooser_destroy(chooser);
}

static void update_window_title(AppState *state)
{
    char title[512];
    TabState *tab = state->current_tab;
    if (!tab) return;

    const char *basename = tab->filename ?
                           g_path_get_basename(tab->filename) : "Untitled";

    if (tab->is_modified) {
        snprintf(title, sizeof(title), "File Roller - %s* (%s) [%d]",
                 basename, get_file_type_name(tab->file_type), state->tab_count);
    } else {
        snprintf(title, sizeof(title), "File Roller - %s (%s) [%d]",
                 basename, get_file_type_name(tab->file_type), state->tab_count);
    }

    gtk_window_set_title(GTK_WINDOW(state->window), title);
}

/* ========== PDF/VIDEO/ARCHIVE FUNCTIONS ========== */

static void load_pdf_file(TabState *tab, const char *filename)
{
    if (!filename || !tab) return;

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "PDF File: %s\n\nSize: %ld bytes\n\n"
                 "For PDF viewing, use:\n  • evince (GNOME Document Viewer)\n  • okular (KDE)\n  • qpdfview\n\n"
                 "To open this file:\n$ evince \"%s\"",
                 g_path_get_basename(filename),
                 statbuf.st_size,
                 filename);
        gtk_label_set_text(GTK_LABEL(tab->pdf_label), msg);
        gtk_label_set_line_wrap(GTK_LABEL(tab->pdf_label), TRUE);
    }
}

static void load_video_file(TabState *tab, const char *filename)
{
    if (!filename || !tab) return;

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "Video File: %s\n\nSize: %ld bytes\n\n"
                 "For video playback, use:\n  • vlc (VLC Media Player)\n  • mpv\n  • ffplay\n\n"
                 "To open this file:\n$ vlc \"%s\"",
                 g_path_get_basename(filename),
                 statbuf.st_size,
                 filename);
        gtk_label_set_text(GTK_LABEL(tab->video_label), msg);
        gtk_label_set_line_wrap(GTK_LABEL(tab->video_label), TRUE);
    }
}

static void load_archive_file(TabState *tab, const char *filename)
{
    if (!filename || !tab) return;

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "Archive File: %s\n\nSize: %ld bytes\n\n"
                 "For archive extraction, use:\n  • file-roller (GNOME)\n  • ark (KDE)\n  • Xarchiver\n\n"
                 "To extract this file:\n$ tar -xf \"%s\"",
                 g_path_get_basename(filename),
                 statbuf.st_size,
                 filename);
        gtk_label_set_text(GTK_LABEL(tab->archive_tree), msg);
    }
}

static void load_unknown_file(TabState *tab, const char *filename)
{
    if (!filename || !tab) return;

    GError *error = NULL;
    gchar *contents = NULL;
    gsize length = 0;

    if (g_file_get_contents(filename, &contents, &length, &error)) {
        gboolean is_text = TRUE;

        int null_count = 0;
        for (gsize i = 0; i < length && i < 8192; i++) {
            if (contents[i] == '\0') {
                null_count++;
            }
        }

        if (null_count > length / 100) {
            is_text = FALSE;
        }

        if (is_text && length > 0) {
            gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->text_view), FALSE);
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view)),
                                     contents, -1);
        } else if (length == 0) {
            gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->text_view), FALSE);
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view)),
                                     "[Empty file]", -1);
        } else {
            gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->text_view), FALSE);
            gchar *hex_preview = g_malloc(1024);
            gchar *ptr = hex_preview;
            int bytes_to_show = (length > 256) ? 256 : length;

            ptr += snprintf(ptr, 1024, "Binary File: %s\nSize: %ld bytes\n\nFirst %d bytes (hex):\n\n",
                           g_path_get_basename(filename), length, bytes_to_show);

            for (int i = 0; i < bytes_to_show; i += 16) {
                ptr += snprintf(ptr, 1024, "%04X: ", i);
                for (int j = 0; j < 16 && (i + j) < bytes_to_show; j++) {
                    ptr += snprintf(ptr, 1024, "%02X ", (unsigned char)contents[i + j]);
                }
                ptr += snprintf(ptr, 1024, "\n");
            }

            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view)),
                                     hex_preview, -1);
            g_free(hex_preview);
        }
        g_free(contents);
    } else if (error) {
        gchar error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Error loading file: %s",
                 error->message);
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view)),
                                 error_msg, -1);
        g_error_free(error);
        return;
    }
}

static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                   gint x, gint y, GtkSelectionData *data,
                                   guint info, guint time, gpointer user_data)
{
    AppState *state = (AppState *)user_data;

    const guchar *drop_data = gtk_selection_data_get_data(data);
    if (!drop_data) return;

    gchar *file_path = NULL;
    if (g_str_has_prefix((const gchar *)drop_data, "file://")) {
        file_path = g_filename_from_uri((const gchar *)drop_data, NULL, NULL);
    } else {
        file_path = g_strdup((const gchar *)drop_data);
    }

    if (file_path) {
        gchar *trimmed = g_strstrip(file_path);
        open_file(state, trimmed);
    }

    gtk_drag_finish(context, TRUE, FALSE, time);
}

/* ========== GENERIC FILE FUNCTIONS ========== */

/**
 * Open file in a new tab
 */
static void open_file(AppState *state, const char *filename)
{
    if (!filename) return;

    /* Create new tab for the file */
    TabState *new_tab = create_new_tab(state);
    state->tabs = g_list_append(state->tabs, new_tab);
    state->tab_count++;

    new_tab->filename = g_strdup(filename);
    new_tab->folder = g_strdup(g_path_get_dirname(filename));
    new_tab->file_type = get_file_type(filename);
    new_tab->is_modified = FALSE;
    new_tab->text_modified = FALSE;

    /* Add tab to notebook */
    const char *tab_label = g_path_get_basename(filename);
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), new_tab->stack,
                            gtk_label_new(tab_label));
    gtk_widget_show_all(new_tab->stack);

    /* Switch to new tab and load file */
    switch_to_tab(state, new_tab);

    switch (new_tab->file_type) {
        case FILE_TYPE_IMAGE:
            gtk_stack_set_visible_child_name(GTK_STACK(new_tab->stack), "image");
            load_image(new_tab, filename);
            break;
        case FILE_TYPE_TEXT:
            gtk_stack_set_visible_child_name(GTK_STACK(new_tab->stack), "text");
            load_text_file(new_tab, filename);
            break;
        case FILE_TYPE_PDF:
            gtk_stack_set_visible_child_name(GTK_STACK(new_tab->stack), "pdf");
            load_pdf_file(new_tab, filename);
            break;
        case FILE_TYPE_VIDEO:
            gtk_stack_set_visible_child_name(GTK_STACK(new_tab->stack), "video");
            load_video_file(new_tab, filename);
            break;
        case FILE_TYPE_AUDIO:
        case FILE_TYPE_ARCHIVE:
            gtk_stack_set_visible_child_name(GTK_STACK(new_tab->stack), "archive");
            load_archive_file(new_tab, filename);
            break;
        default:
            gtk_stack_set_visible_child_name(GTK_STACK(new_tab->stack), "text");
            load_unknown_file(new_tab, filename);
            break;
    }

    update_window_title(state);
}

/* ========== TOOLBAR CALLBACKS ========== */

static void on_zoom_in(GtkWidget *widget, AppState *state)
{
    if (state->current_tab && state->current_tab->file_type == FILE_TYPE_IMAGE) {
        zoom_in(state->current_tab);
    }
}

static void on_zoom_out(GtkWidget *widget, AppState *state)
{
    if (state->current_tab && state->current_tab->file_type == FILE_TYPE_IMAGE) {
        zoom_out(state->current_tab);
    }
}

static void on_fit_window(GtkWidget *widget, AppState *state)
{
    if (state->current_tab && state->current_tab->file_type == FILE_TYPE_IMAGE) {
        fit_image_to_window(state->current_tab);
    }
}

static void on_open_file(GtkWidget *widget, AppState *state)
{
    open_file_dialog(state);
}

static void on_save_file(GtkWidget *widget, AppState *state)
{
    save_file(state);
}

static void on_save_file_as(GtkWidget *widget, AppState *state)
{
    save_file_as(state);
}

static void on_show_properties(GtkWidget *widget, AppState *state)
{
    TabState *tab = state->current_tab;
    if (!tab || !tab->filename) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "No file is currently open");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    struct stat statbuf;
    if (stat(tab->filename, &statbuf) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Could not get file information");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    char msg[1024];
    char time_buf[64];
    struct tm *tm_info = localtime(&statbuf.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    snprintf(msg, sizeof(msg),
             "File: %s\n\n"
             "Size: %ld bytes (%.2f KB)\n"
             "Type: %s\n"
             "Modified: %s\n"
             "Permissions: %o\n"
             "Owner UID: %d\n"
             "Group GID: %d",
             g_path_get_basename(tab->filename),
             statbuf.st_size,
             (double)statbuf.st_size / 1024.0,
             get_file_type_name(tab->file_type),
             time_buf,
             statbuf.st_mode & 0777,
             statbuf.st_uid,
             statbuf.st_gid);

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "File Properties");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_copy_text(GtkWidget *widget, AppState *state)
{
    TabState *tab = state->current_tab;
    if (!tab) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (text && strlen(text) > 0) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, text, -1);
    }

    if (text) g_free(text);
}

/**
 * Callback when notebook page is switched
 */
static void on_notebook_switch_page(GtkNotebook *notebook, GtkWidget *page,
                                    guint page_num, AppState *state)
{
    /* Find which tab this page belongs to */
    for (GList *node = state->tabs; node != NULL; node = node->next) {
        TabState *tab = (TabState *)node->data;
        if (tab->stack == page) {
            state->current_tab = tab;
            update_window_title(state);
            return;
        }
    }
}

/* ========== WINDOW SETUP ========== */

static GtkWidget *create_custom_titlebar(AppState *state)
{
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(header, -1, 30);
    gtk_widget_set_name(header, "file-roller-header");

    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<b>File Roller</b>");
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(title_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(header), title_label, TRUE, TRUE, 0);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(header), button_box, FALSE, FALSE, 5);

    GtkWidget *min_btn = gtk_button_new_with_label("—");
    gtk_widget_set_size_request(min_btn, 25, 20);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), state->window);
    gtk_box_pack_start(GTK_BOX(button_box), min_btn, FALSE, FALSE, 0);

    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 25, 20);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), state->window);
    g_signal_connect(state->window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(button_box), max_btn, FALSE, FALSE, 0);

    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 25, 20);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), state->window);
    gtk_box_pack_start(GTK_BOX(button_box), close_btn, FALSE, FALSE, 0);

    return header;
}

static GtkWidget *create_toolbar(AppState *state)
{
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_margin_top(toolbar, 5);
    gtk_widget_set_margin_bottom(toolbar, 5);
    gtk_widget_set_margin_start(toolbar, 10);
    gtk_widget_set_margin_end(toolbar, 10);

    GtkWidget *btn_open = gtk_button_new_with_label("📁 Open");
    gtk_widget_set_tooltip_text(btn_open, "Open a file");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_file), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_open, FALSE, FALSE, 0);

    GtkWidget *btn_save = gtk_button_new_with_label("💾 Save");
    gtk_widget_set_tooltip_text(btn_save, "Save file");
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_file), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_save, FALSE, FALSE, 0);

    GtkWidget *btn_save_as = gtk_button_new_with_label("📋 Save As");
    gtk_widget_set_tooltip_text(btn_save_as, "Save as new file");
    g_signal_connect(btn_save_as, "clicked", G_CALLBACK(on_save_file_as), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_save_as, FALSE, FALSE, 0);

    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar), sep1, FALSE, FALSE, 5);

    GtkWidget *btn_zoom_in = gtk_button_new_with_label("🔍+");
    gtk_widget_set_tooltip_text(btn_zoom_in, "Zoom In");
    GtkWidget *btn_zoom_out = gtk_button_new_with_label("🔍-");
    gtk_widget_set_tooltip_text(btn_zoom_out, "Zoom Out");
    GtkWidget *btn_fit = gtk_button_new_with_label("📐 Fit");
    gtk_widget_set_tooltip_text(btn_fit, "Fit to Window");

    g_signal_connect(btn_zoom_in, "clicked", G_CALLBACK(on_zoom_in), state);
    g_signal_connect(btn_zoom_out, "clicked", G_CALLBACK(on_zoom_out), state);
    g_signal_connect(btn_fit, "clicked", G_CALLBACK(on_fit_window), state);

    gtk_box_pack_start(GTK_BOX(toolbar), btn_zoom_in, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_zoom_out, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_fit, FALSE, FALSE, 0);

    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar), sep2, FALSE, FALSE, 5);

    GtkWidget *btn_copy = gtk_button_new_with_label("📋 Copy");
    gtk_widget_set_tooltip_text(btn_copy, "Copy Text");
    g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_copy_text), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_copy, FALSE, FALSE, 0);

    GtkWidget *btn_props = gtk_button_new_with_label("ℹ️ Props");
    gtk_widget_set_tooltip_text(btn_props, "File Properties");
    g_signal_connect(btn_props, "clicked", G_CALLBACK(on_show_properties), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_props, FALSE, FALSE, 0);

    return toolbar;
}

static void create_window(AppState *state)
{
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "File Roller");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 900, 700);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(state->window), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(state->window), FALSE);

    gtk_widget_add_events(state->window, GDK_BUTTON_PRESS_MASK |
                                          GDK_BUTTON_RELEASE_MASK |
                                          GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->window, "button-press-event", G_CALLBACK(on_button_press), state);
    g_signal_connect(state->window, "button-release-event", G_CALLBACK(on_button_release), state);
    g_signal_connect(state->window, "motion-notify-event", G_CALLBACK(on_motion_notify), state);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), main_vbox);

    GtkWidget *titlebar = create_custom_titlebar(state);
    gtk_box_pack_start(GTK_BOX(main_vbox), titlebar, FALSE, FALSE, 0);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_vbox), sep, FALSE, FALSE, 0);

    GtkWidget *toolbar = create_toolbar(state);
    gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);

    /* Create notebook for tabs */
    state->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(state->notebook), TRUE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(state->notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(state->notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(main_vbox), state->notebook, TRUE, TRUE, 0);
    g_signal_connect(state->notebook, "switch-page", G_CALLBACK(on_notebook_switch_page), state);

    state->statusbar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(main_vbox), state->statusbar, FALSE, FALSE, 0);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; }"
        "#file-roller-header { background-color: #1a1e24; padding: 2px; }"
        "#file-roller-header button { background-color: #2a323a; color: #00ff88; border: none; min-height: 20px; min-width: 25px; }"
        "#file-roller-header button:hover { background-color: #3a424a; }"
        "#file-roller-header button:active { background-color: #00ff88; color: #0b0f14; }"
        "button { background-color: #1e2429; color: #00ff88; border: 1px solid #00ff88; }"
        "button:hover { background-color: #2a323a; }"
        "label { color: #ffffff; }"
        "textview { background-color: #1a1a1a; color: #ffffff; }"
        "textview text { background-color: #1a1a1a; color: #ffffff; }"
        "statusbar { background-color: #0b0f14; color: #00ff88; }"
        "frame { border-color: #2a323a; }"
        "scrolledwindow { border: none; background-color: #0b0f14; }"
        "scrollbar { background-color: #1e2429; }"
        "scrollbar slider { background-color: #62316b; border-radius: 4px; min-width: 8px; min-height: 8px; }"
        "scrollbar slider:hover { background-color: #7a3b8b; }"
        "scrollbar slider:active { background-color: #9a4bab; }"
        "scrollbar trough { background-color: #2a323a; border-radius: 4px; }"
        "notebook { background-color: #0b0f14; }"
        "notebook tab { background-color: #1a1e24; color: #00ff88; padding: 5px 10px; }",
        -1, NULL);

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    g_signal_connect(state->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(state->window, "key-press-event", G_CALLBACK(on_key_press), state);

    static const GtkTargetEntry target_entries[] = {
        { "text/uri-list", 0, 0 }
    };
    gtk_drag_dest_set(state->window, GTK_DEST_DEFAULT_ALL,
                      target_entries, G_N_ELEMENTS(target_entries),
                      GDK_ACTION_COPY);

    g_signal_connect(state->window, "drag-data-received",
                    G_CALLBACK(on_drag_data_received), state);

    gtk_widget_show_all(state->window);
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, AppState *state)
{
    if (!state->current_tab) return FALSE;

    if (event->state & GDK_CONTROL_MASK) {
        switch (event->keyval) {
            case GDK_KEY_plus:
            case GDK_KEY_equal:
                if (state->current_tab->file_type == FILE_TYPE_IMAGE) {
                    zoom_in(state->current_tab);
                }
                return TRUE;
            case GDK_KEY_minus:
                if (state->current_tab->file_type == FILE_TYPE_IMAGE) {
                    zoom_out(state->current_tab);
                }
                return TRUE;
            case GDK_KEY_0:
                if (state->current_tab->file_type == FILE_TYPE_IMAGE) {
                    fit_image_to_window(state->current_tab);
                }
                return TRUE;
            case GDK_KEY_o:
                on_open_file(NULL, state);
                return TRUE;
            case GDK_KEY_s:
                save_file(state);
                return TRUE;
            case GDK_KEY_c:
                on_copy_text(NULL, state);
                return TRUE;
            case GDK_KEY_w:
                close_current_tab(state);
                return TRUE;
            default:
                break;
        }
    }

    if (event->state & GDK_MOD1_MASK) {
        switch (event->keyval) {
            case GDK_KEY_F4:
                gtk_main_quit();
                return TRUE;
            default:
                break;
        }
    }

    return FALSE;
}

/* ========== MAIN APPLICATION ========== */

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    AppState state = {0};
    state.tabs = NULL;
    state.current_tab = NULL;
    state.tab_count = 0;

    create_window(&state);

    /* Create initial tab */
    TabState *initial_tab = create_new_tab(&state);
    state.tabs = g_list_append(state.tabs, initial_tab);
    state.current_tab = initial_tab;
    state.tab_count = 1;

    /* Add initial tab to notebook with label */
    GtkWidget *initial_label = gtk_label_new("Welcome");
    gtk_notebook_append_page(GTK_NOTEBOOK(state.notebook), initial_tab->stack, initial_label);
    gtk_widget_show_all(initial_tab->stack);

    /* Show welcome page by default */
    gtk_stack_set_visible_child_name(GTK_STACK(initial_tab->stack), "welcome");

    update_window_title(&state);

    /* If file argument provided, open it */
    if (argc > 1) {
        open_file(&state, argv[1]);
    }

    gtk_main();

    /* Clean up all tabs */
    for (GList *node = state.tabs; node != NULL; node = node->next) {
        close_tab_data((TabState *)node->data);
    }
    g_list_free(state.tabs);

    return 0;
}
