#ifndef PASTE_H
#define PASTE_H

#include <gtk/gtk.h>

/*
 * paste.h
 *
 * Provides a lightweight paste API for applications that need to read
 * the system clipboard contents. This module separates paste logic from
 * application UI so multiple tools can reuse the same clipboard behavior.
 */

gchar *paste_text_from_clipboard(void);
GdkPixbuf *paste_image_from_clipboard(void);
gchar *paste_uri_list_from_clipboard(void);

#endif // PASTE_H
