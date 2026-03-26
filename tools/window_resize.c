#include "window_resize.h"
#include <gtk/gtk.h>

/**
 * Detect which resize edge or corner is near the cursor using absolute screen coordinates.
 *
 * @param window        GTK window to query for position and size.
 * @param cursor_x_root Cursor X coordinate in root window space.
 * @param cursor_y_root Cursor Y coordinate in root window space.
 * @return Bitmask combining RESIZE_LEFT, RESIZE_RIGHT, RESIZE_TOP, RESIZE_BOTTOM,
 *         or a corner constant (e.g., RESIZE_TOP_LEFT) for combined edges.
 */
int detect_resize_edge_absolute(GtkWindow *window, int cursor_x_root, int cursor_y_root) {
    int resize_edge = RESIZE_NONE;
    int window_x, window_y, window_width, window_height;

    gtk_window_get_position(window, &window_x, &window_y);
    gtk_window_get_size(window, &window_width, &window_height);

    /* Calculate relative position within window */
    int rel_x = cursor_x_root - window_x;
    int rel_y = cursor_y_root - window_y;

    /* Detect left/right */
    if (rel_x < RESIZE_BORDER) {
        resize_edge |= RESIZE_LEFT;
    } else if (rel_x > window_width - RESIZE_BORDER) {
        resize_edge |= RESIZE_RIGHT;
    }

    /* Detect top/bottom */
    if (rel_y < RESIZE_BORDER) {
        resize_edge |= RESIZE_TOP;
    } else if (rel_y > window_height - RESIZE_BORDER) {
        resize_edge |= RESIZE_BOTTOM;
    }

    return resize_edge;
}

/**
 * Detect which resize edge or corner is near the cursor using local window coordinates.
 *
 * @param cursor_x      Cursor X coordinate relative to window.
 * @param cursor_y      Cursor Y coordinate relative to window.
 * @param window_width  Current window width.
 * @param window_height Current window height.
 * @return Bitmask combining RESIZE_LEFT, RESIZE_RIGHT, RESIZE_TOP, RESIZE_BOTTOM,
 *         or a corner constant for combined edges.
 */
int detect_resize_edge(int cursor_x, int cursor_y, int window_width, int window_height) {
    int resize_edge = RESIZE_NONE;

    /* Detect left/right */
    if (cursor_x < RESIZE_BORDER) {
        resize_edge |= RESIZE_LEFT;
    } else if (cursor_x > window_width - RESIZE_BORDER) {
        resize_edge |= RESIZE_RIGHT;
    }

    /* Detect top/bottom */
    if (cursor_y < RESIZE_BORDER) {
        resize_edge |= RESIZE_TOP;
    } else if (cursor_y > window_height - RESIZE_BORDER) {
        resize_edge |= RESIZE_BOTTOM;
    }

    return resize_edge;
}

/**
 * Update the window cursor based on the detected resize edge.
 *
 * @param widget      Widget whose window cursor will be changed.
 * @param resize_edge Resize edge bitmask (RESIZE_LEFT, RESIZE_TOP_RIGHT, etc.).
 */
void update_resize_cursor(GtkWidget *widget, int resize_edge) {
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = NULL;

    switch (resize_edge) {
        case RESIZE_LEFT:
        case RESIZE_RIGHT:
            cursor = gdk_cursor_new_from_name(display, "ew-resize");
            break;
        case RESIZE_TOP:
        case RESIZE_BOTTOM:
            cursor = gdk_cursor_new_from_name(display, "ns-resize");
            break;
        case RESIZE_TOP_LEFT:
        case RESIZE_BOTTOM_RIGHT:
            cursor = gdk_cursor_new_from_name(display, "nwse-resize");
            break;
        case RESIZE_TOP_RIGHT:
        case RESIZE_BOTTOM_LEFT:
            cursor = gdk_cursor_new_from_name(display, "nesw-resize");
            break;
        default:
            cursor = gdk_cursor_new_from_name(display, "default");
            break;
    }

    if (cursor) {
        gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
        g_object_unref(cursor);
    }
}

/**
 * Apply window resize based on the drag delta and resize edge.
 *
 * @param window         GTK window to resize.
 * @param resize_edge    Bitmask indicating which edges are being dragged.
 * @param delta_x        Horizontal mouse movement since drag start.
 * @param delta_y        Vertical mouse movement since drag start.
 * @param current_width  Current window width before resize.
 * @param current_height Current window height before resize.
 */
void apply_window_resize(GtkWindow *window, int resize_edge,
                         int delta_x, int delta_y,
                         int current_width, int current_height) {
    int new_width = current_width;
    int new_height = current_height;
    int new_x = 0, new_y = 0;

    /* Get current window position */
    gtk_window_get_position(window, &new_x, &new_y);

    /* Calculate new dimensions based on which edge is being resized */
    if (resize_edge & RESIZE_LEFT) {
        new_width -= delta_x;
        new_x += delta_x;
    } else if (resize_edge & RESIZE_RIGHT) {
        new_width += delta_x;
    }

    if (resize_edge & RESIZE_TOP) {
        new_height -= delta_y;
        new_y += delta_y;
    } else if (resize_edge & RESIZE_BOTTOM) {
        new_height += delta_y;
    }

    /* Enforce minimum size */
    if (new_width < 200) {
        new_width = 200;
        if (resize_edge & RESIZE_LEFT) {
            new_x -= (200 - new_width);
        }
    }
    if (new_height < 150) {
        new_height = 150;
        if (resize_edge & RESIZE_TOP) {
            new_y -= (150 - new_height);
        }
    }

    /* Apply new position if left or top edge was resized */
    if (resize_edge & (RESIZE_LEFT | RESIZE_TOP)) {
        gtk_window_move(window, new_x, new_y);
    }

    /* Resize the window */
    gtk_window_resize(window, new_width, new_height);
}