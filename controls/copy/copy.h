#ifndef COPY_H
#define COPY_H

#include <gtk/gtk.h>

/*
 * copy.h
 *
 * Exposes a simple copy API for text, images, and URI lists.
 * This helper is used by tools that need to place content into the
 * system clipboard with a consistent, reusable interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Copy raw or plain text into the clipboard. */
void copy_text_to_clipboard(const gchar *text, gboolean plain_text);

/* Copy an image into the clipboard. */
void copy_image_to_clipboard(GdkPixbuf *image);

/* Copy a URI list into the clipboard for file sharing operations. */
void copy_uri_list_to_clipboard(const gchar *uri_list);

#ifdef __cplusplus
}
#endif

#endif // COPY_H
