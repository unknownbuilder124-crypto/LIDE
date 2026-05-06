#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <gtk/gtk.h>

/*
 * clipboard.h
 *
 * Defines the clipboard history model and the public API used by the
 * clipboard manager window.
 */

typedef enum {
    CLIPBOARD_TYPE_TEXT,
    CLIPBOARD_TYPE_IMAGE,
    CLIPBOARD_TYPE_URI_LIST
} ClipboardItemType;

typedef struct {
    ClipboardItemType type;
    gchar *label;
    gchar *text;
    gchar *uri_list;
    GdkPixbuf *image;
    gboolean pinned;
} ClipboardItem;

typedef struct {
    GtkWidget *window;
    GtkWidget *history_list;
    GtkClipboard *clipboard;
    GList *history;
    gchar *last_signature;
} ClipboardApp;

void add_clipboard_history_item(ClipboardApp *app, ClipboardItem *item);
void update_clipboard_history_view(ClipboardApp *app);
ClipboardItem *clipboard_item_new_text(const gchar *text);
ClipboardItem *clipboard_item_new_image(GdkPixbuf *image);
ClipboardItem *clipboard_item_new_uri(const gchar *uri_list);
void clipboard_item_free(ClipboardItem *item);

#endif // CLIPBOARD_H

