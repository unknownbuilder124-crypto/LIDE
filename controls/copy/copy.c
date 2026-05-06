/*
 * copy.c
 *
 * Implements clipboard copy helpers used throughout the LIDE toolset.
 * The functions here keep clipboard access consistent and avoid repeating
 * clipboard boilerplate in multiple tools.
 */

#include "copy.h"

static GtkClipboard *clipboard_get(void)
{
    return gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
}

void copy_text_to_clipboard(const gchar *text, gboolean plain_text)
{
    if (!text) return;
    GtkClipboard *clipboard = clipboard_get();
    if (plain_text) {
        /* Ensure the clipboard receives plain text without formatting. */
        gchar *plain = g_strdup(text);
        gtk_clipboard_set_text(clipboard, plain, -1);
        g_free(plain);
    } else {
        gtk_clipboard_set_text(clipboard, text, -1);
    }
}

void copy_image_to_clipboard(GdkPixbuf *image)
{
    if (!image) return;
    GtkClipboard *clipboard = clipboard_get();
    gtk_clipboard_set_image(clipboard, image);
}

void copy_uri_list_to_clipboard(const gchar *uri_list)
{
    if (!uri_list) return;
    GtkClipboard *clipboard = clipboard_get();
    gtk_clipboard_set_text(clipboard, uri_list, -1);
}
