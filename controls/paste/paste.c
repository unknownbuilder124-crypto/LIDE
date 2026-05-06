#include <gtk/gtk.h>
#include "paste.h"

/*
 * paste.c
 *
 * Implements paste helpers for reading clipboard contents. The functions
 * return newly allocated values or references that the caller must release.
 *
 * This module is intentionally minimal and forwards all clipboard access
 * through GtkClipboard so the underlying system selection remains consistent.
 */

static GtkClipboard *paste_clipboard_get(void)
{
    return gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
}

/*
 * Returns a duplicate of the current clipboard text, or NULL if there is no
 * text available.
 */
gchar *paste_text_from_clipboard(void)
{
    gchar *text = gtk_clipboard_wait_for_text(paste_clipboard_get());
    if (!text) {
        return NULL;
    }
    gchar *copy = g_strdup(text);
    g_free(text);
    return copy;
}

/*
 * Returns a referenced image from the clipboard, or NULL if there is no image.
 */
GdkPixbuf *paste_image_from_clipboard(void)
{
    GdkPixbuf *image = gtk_clipboard_wait_for_image(paste_clipboard_get());
    if (!image) {
        return NULL;
    }
    return image;
}

/*
 * Returns a duplicate of the current clipboard content interpreted as a URI
 * list, or NULL if the clipboard does not contain text.
 */
gchar *paste_uri_list_from_clipboard(void)
{
    gchar *text = gtk_clipboard_wait_for_text(paste_clipboard_get());
    if (!text) {
        return NULL;
    }
    gchar *copy = g_strdup(text);
    g_free(text);
    return copy;
}
