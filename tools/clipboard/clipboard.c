/*
 * clipboard.c
 *
 * Clipboard Manager application for LIDE. This tool watches the system
 * clipboard, stores the most recent items, and exposes them through a
 * GTK-based history UI. The manager supports text, image, and file URI
 * clipboard entries, plus pinning and plain-text paste behavior.
 */

#include <gtk/gtk.h>
#include <string.h>
#include "clipboard.h"
#include "copy.h"
#include "paste.h"

#define MAX_CLIPBOARD_HISTORY 50
#define PREVIEW_SIZE 64

static gboolean is_uri_list(const gchar *text);

static gchar *make_signature_for_item(ClipboardItem *item)
{
    switch (item->type) {
        case CLIPBOARD_TYPE_TEXT:
            return g_strdup_printf("text:%s", item->text ? item->text : "");
        case CLIPBOARD_TYPE_URI_LIST:
            return g_strdup_printf("uri:%s", item->uri_list ? item->uri_list : "");
        case CLIPBOARD_TYPE_IMAGE:
            return g_strdup_printf("image:%dx%d",
                                   gdk_pixbuf_get_width(item->image),
                                   gdk_pixbuf_get_height(item->image));
    }
    return g_strdup("unknown");
}

static gboolean clipboard_item_equal(const ClipboardItem *a, const ClipboardItem *b)
{
    if (a->type != b->type) return FALSE;
    switch (a->type) {
        case CLIPBOARD_TYPE_TEXT:
            return g_strcmp0(a->text, b->text) == 0;
        case CLIPBOARD_TYPE_URI_LIST:
            return g_strcmp0(a->uri_list, b->uri_list) == 0;
        case CLIPBOARD_TYPE_IMAGE:
            return gdk_pixbuf_get_width(a->image) == gdk_pixbuf_get_width(b->image) &&
                   gdk_pixbuf_get_height(a->image) == gdk_pixbuf_get_height(b->image);
    }
    return FALSE;
}

ClipboardItem *clipboard_item_new_text(const gchar *text)
{
    if (!text) return NULL;
    ClipboardItem *item = g_new0(ClipboardItem, 1);
    item->type = CLIPBOARD_TYPE_TEXT;
    item->text = g_strdup(text);
    item->label = g_strdup_printf("%.*s", 80, text);
    return item;
}

ClipboardItem *clipboard_item_new_uri(const gchar *uri_list)
{
    if (!uri_list) return NULL;
    ClipboardItem *item = g_new0(ClipboardItem, 1);
    item->type = CLIPBOARD_TYPE_URI_LIST;
    item->uri_list = g_strdup(uri_list);
    item->label = g_strdup_printf("Files: %.*s", 60, uri_list);
    return item;
}

ClipboardItem *clipboard_item_new_image(GdkPixbuf *image)
{
    if (!image) return NULL;
    ClipboardItem *item = g_new0(ClipboardItem, 1);
    item->type = CLIPBOARD_TYPE_IMAGE;
    item->image = gdk_pixbuf_copy(image);
    item->label = g_strdup_printf("Image %lux%lu",
                                 gdk_pixbuf_get_width(image),
                                 gdk_pixbuf_get_height(image));
    return item;
}

void clipboard_item_free(ClipboardItem *item)
{
    if (!item) return;
    g_free(item->label);
    g_free(item->text);
    g_free(item->uri_list);
    if (item->image) g_object_unref(item->image);
    g_free(item);
}

static void remove_old_history(ClipboardApp *app)
{
    while (g_list_length(app->history) > MAX_CLIPBOARD_HISTORY) {
        GList *last = g_list_last(app->history);
        ClipboardItem *item = last->data;
        if (item->pinned) {
            break;
        }
        app->history = g_list_delete_link(app->history, last);
        clipboard_item_free(item);
    }
}

void add_clipboard_history_item(ClipboardApp *app, ClipboardItem *item)
{
    if (!item || !app) return;

    for (GList *iter = app->history; iter; iter = iter->next) {
        ClipboardItem *existing = iter->data;
        if (clipboard_item_equal(existing, item)) {
            existing->pinned = existing->pinned || item->pinned;
            app->history = g_list_delete_link(app->history, iter);
            app->history = g_list_prepend(app->history, existing);
            clipboard_item_free(item);
            return;
        }
    }

    app->history = g_list_prepend(app->history, item);
    remove_old_history(app);
}

/*
 * Copy the selected history item back into the system clipboard.
 *
 * @param item The history item to copy.
 * @param plain If TRUE, force plain-text output for supported item types.
 */
static void copy_item_to_clipboard(ClipboardItem *item, gboolean plain)
{
    if (!item) return;
    switch (item->type) {
        case CLIPBOARD_TYPE_TEXT:
            copy_text_to_clipboard(item->text, plain);
            break;
        case CLIPBOARD_TYPE_IMAGE:
            copy_image_to_clipboard(item->image);
            break;
        case CLIPBOARD_TYPE_URI_LIST:
            if (plain) {
                copy_text_to_clipboard(item->uri_list, TRUE);
            } else {
                copy_uri_list_to_clipboard(item->uri_list);
            }
            break;
    }
}

static void paste_current_clipboard(ClipboardApp *app)
{
    /*
     * Force a one-shot clipboard read into the manager. This is useful when
     * the user presses Ctrl+V inside the manager and wants the latest system
     * clipboard content to appear immediately in the history view.
     */
    if (!app) return;

    gchar *text = paste_text_from_clipboard();
    if (text) {
        ClipboardItem *item = is_uri_list(text)
            ? clipboard_item_new_uri(text)
            : clipboard_item_new_text(text);
        g_free(text);
        if (item) {
            add_clipboard_history_item(app, item);
            update_clipboard_history_view(app);
        }
        return;
    }

    GdkPixbuf *image = paste_image_from_clipboard();
    if (image) {
        ClipboardItem *item = clipboard_item_new_image(image);
        g_object_unref(image);
        if (item) {
            add_clipboard_history_item(app, item);
            update_clipboard_history_view(app);
        }
    }
}

static gboolean on_clipboard_manager_key_press(GtkWidget *widget,
                                               GdkEventKey *event,
                                               gpointer user_data)
{
    ClipboardApp *app = user_data;
    if (!app) return FALSE;

    if (!(event->state & GDK_CONTROL_MASK)) {
        return FALSE;
    }

    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->history_list));
    ClipboardItem *item = NULL;
    if (row) {
        item = g_object_get_data(G_OBJECT(row), "clipboard-item");
    }

    if (event->keyval == GDK_KEY_c && item) {
        copy_item_to_clipboard(item, FALSE);
        return TRUE;
    }

    if (event->keyval == GDK_KEY_v) {
        if (item) {
            copy_item_to_clipboard(item, TRUE);
            return TRUE;
        }
        paste_current_clipboard(app);
        return TRUE;
    }

    return FALSE;
}

static void on_plain_button_clicked(GtkButton *button, gpointer data)
{
    ClipboardItem *item = data;
    copy_item_to_clipboard(item, TRUE);
}

typedef struct {
    ClipboardApp *app;
    ClipboardItem *item;
} PinPayload;

static void on_pin_button_clicked(GtkButton *button, gpointer data)
{
    PinPayload *payload = data;
    ClipboardApp *app = payload->app;
    ClipboardItem *item = payload->item;
    item->pinned = !item->pinned;
    update_clipboard_history_view(app);
}

/*
 * Callback for the "Copy" button - copies item to clipboard with full formatting.
 *
 * @param button Button widget
 * @param data ClipboardItem pointer to copy
 */
static void on_copy_button_clicked(GtkButton *button, gpointer data)
{
    ClipboardItem *item = data;
    copy_item_to_clipboard(item, FALSE);
}


void update_clipboard_history_view(ClipboardApp *app)
{
    if (!app || !GTK_IS_LIST_BOX(app->history_list)) return;

    GList *children = gtk_container_get_children(GTK_CONTAINER(app->history_list));
    for (GList *iter = children; iter; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    GList *sorted = NULL;
    for (GList *iter = app->history; iter; iter = iter->next) {
        ClipboardItem *item = iter->data;
        if (item->pinned) sorted = g_list_append(sorted, item);
    }
    for (GList *iter = app->history; iter; iter = iter->next) {
        ClipboardItem *item = iter->data;
        if (!item->pinned) sorted = g_list_append(sorted, item);
    }

    for (GList *iter = sorted; iter; iter = iter->next) {
        ClipboardItem *item = iter->data;
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_top(box, 8);
        gtk_widget_set_margin_bottom(box, 8);
        gtk_widget_set_margin_start(box, 10);
        gtk_widget_set_margin_end(box, 10);

        GtkWidget *icon = NULL;
        if (item->type == CLIPBOARD_TYPE_IMAGE) {
            GdkPixbuf *preview = gdk_pixbuf_scale_simple(item->image, PREVIEW_SIZE, PREVIEW_SIZE, GDK_INTERP_BILINEAR);
            icon = gtk_image_new_from_pixbuf(preview);
            g_object_unref(preview);
        } else if (item->type == CLIPBOARD_TYPE_URI_LIST) {
            icon = gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_DIALOG);
        } else {
            icon = gtk_image_new_from_icon_name("accessories-text-editor", GTK_ICON_SIZE_DIALOG);
        }

        GtkWidget *label = gtk_label_new(item->label);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_widget_set_hexpand(label, TRUE);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 80);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *btn_copy = gtk_button_new_with_label("Copy");
        GtkWidget *btn_plain = gtk_button_new_with_label("Plain text");
        GtkWidget *btn_pin = gtk_button_new_with_label(item->pinned ? "Unpin" : "Pin");

        g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_copy_button_clicked), item);
        g_signal_connect(btn_plain, "clicked", G_CALLBACK(on_plain_button_clicked), item);
        PinPayload *pin_data = g_new0(PinPayload, 1);
        pin_data->app = app;
        pin_data->item = item;
        g_object_set_data_full(G_OBJECT(btn_pin), "pin-data", pin_data, g_free);
        g_signal_connect(btn_pin, "clicked", G_CALLBACK(on_pin_button_clicked), pin_data);

        gtk_box_pack_start(GTK_BOX(button_box), btn_copy, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(button_box), btn_plain, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(button_box), btn_pin, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, FALSE, 0);

        if (item->pinned) {
            GtkWidget *pin_label = gtk_label_new("📌");
            gtk_box_pack_start(GTK_BOX(box), pin_label, FALSE, FALSE, 0);
        }

        g_object_set_data(G_OBJECT(row), "clipboard-item", item);
        gtk_container_add(GTK_CONTAINER(row), box);
        gtk_container_add(GTK_CONTAINER(app->history_list), row);
    }
    g_list_free(sorted);
    gtk_widget_show_all(app->history_list);
}

static gboolean is_uri_list(const gchar *text)
{
    if (!text) return FALSE;
    gchar **lines = g_strsplit(text, "\n", -1);
    gboolean all_file = TRUE;
    for (gchar **line = lines; *line; line++) {
        if (**line == '\0') continue;
        if (!g_str_has_prefix(*line, "file://")) {
            all_file = FALSE;
            break;
        }
    }
    g_strfreev(lines);
    return all_file;
}

static gboolean handle_clipboard_changed(gpointer user_data)
{
    ClipboardApp *app = user_data;
    if (!app || !app->clipboard) return TRUE;

    gchar *text = gtk_clipboard_wait_for_text(app->clipboard);
    ClipboardItem *item = NULL;
    if (text) {
        if (is_uri_list(text)) {
            item = clipboard_item_new_uri(text);
        } else {
            item = clipboard_item_new_text(text);
        }
        g_free(text);
    } else {
        GdkPixbuf *image = gtk_clipboard_wait_for_image(app->clipboard);
        if (image) {
            item = clipboard_item_new_image(image);
            g_object_unref(image);
        }
    }

    if (item) {
        gchar *sig = make_signature_for_item(item);
        if (!app->last_signature || g_strcmp0(app->last_signature, sig) != 0) {
            add_clipboard_history_item(app, item);
            update_clipboard_history_view(app);
            g_free(app->last_signature);
            app->last_signature = sig;
        } else {
            clipboard_item_free(item);
            g_free(sig);
        }
    }

    return TRUE;
}

static void on_close_button_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    GtkWindow *window = GTK_WINDOW(data);
    gtk_window_close(window);
}

static void on_minimize_button_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    GtkWindow *window = GTK_WINDOW(data);
    gtk_window_iconify(window);
}

static GtkWidget *build_toolbar(ClipboardApp *app)
{
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(toolbar, 10);
    gtk_widget_set_margin_end(toolbar, 10);
    gtk_widget_set_margin_top(toolbar, 10);
    gtk_widget_set_margin_bottom(toolbar, 10);

    GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *title = gtk_label_new("Clipboard History");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    PangoFontDescription *font = pango_font_description_from_string("Sans Bold 14");
    gtk_widget_override_font(title, font);
    pango_font_description_free(font);
    gtk_widget_set_hexpand(title, TRUE);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *min_button = gtk_button_new_with_label("─");
    GtkWidget *close_button = gtk_button_new_with_label("✕");
    gtk_widget_set_tooltip_text(min_button, "Minimize clipboard manager");
    gtk_widget_set_tooltip_text(close_button, "Exit clipboard manager");
    g_signal_connect(min_button, "clicked", G_CALLBACK(on_minimize_button_clicked), app->window);
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_button_clicked), app->window);
    gtk_box_pack_start(GTK_BOX(button_box), min_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), close_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(top_row), title, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(top_row), button_box, FALSE, FALSE, 0);

    GtkWidget *description = gtk_label_new("Super+V opens this manager. Use Ctrl+C to copy the selected history item, or Ctrl+V to paste it back into the clipboard.");
    gtk_label_set_line_wrap(GTK_LABEL(description), TRUE);
    gtk_label_set_xalign(GTK_LABEL(description), 0.0);

    gtk_box_pack_start(GTK_BOX(toolbar), top_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), description, FALSE, FALSE, 0);
    return toolbar;
}

static void activate(GtkApplication *app, gpointer user_data)
{
    ClipboardApp *state = user_data;
    state->clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "Clipboard Manager");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 600, 520);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), root_box);

    GtkWidget *toolbar = build_toolbar(state);
    gtk_box_pack_start(GTK_BOX(root_box), toolbar, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);

    state->history_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state->history_list), GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(scrolled), state->history_list);
    gtk_box_pack_start(GTK_BOX(root_box), scrolled, TRUE, TRUE, 0);

    gtk_widget_add_events(state->window, GDK_KEY_PRESS_MASK);
    g_signal_connect(state->window, "key-press-event", G_CALLBACK(on_clipboard_manager_key_press), state);

    gtk_widget_show_all(state->window);
    update_clipboard_history_view(state);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("org.blackline.clipboard", G_APPLICATION_FLAGS_NONE);
    ClipboardApp state = {0};
    state.history = NULL;
    state.last_signature = NULL;

    g_signal_connect(app, "activate", G_CALLBACK(activate), &state);
    g_timeout_add_seconds(1, handle_clipboard_changed, &state);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_list_free_full(state.history, (GDestroyNotify)clipboard_item_free);
    g_free(state.last_signature);
    g_object_unref(app);
    return status;
}
