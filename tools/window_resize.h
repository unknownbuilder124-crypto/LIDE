#ifndef BLACKLINE_WINDOW_RESIZE_H
#define BLACKLINE_WINDOW_RESIZE_H

#include <gtk/gtk.h>

// Resize edge detection constants
#define RESIZE_BORDER 8  // pixels from edge to detect resize
#define RESIZE_NONE 0
#define RESIZE_LEFT 1
#define RESIZE_RIGHT 2
#define RESIZE_TOP 4
#define RESIZE_BOTTOM 8
#define RESIZE_TOP_LEFT 5
#define RESIZE_TOP_RIGHT 6
#define RESIZE_BOTTOM_LEFT 9
#define RESIZE_BOTTOM_RIGHT 10

/**
 * Detect which resize edge or corner is near the cursor using absolute screen coordinates.
 *
 * @param window        GTK window to query for position and size.
 * @param cursor_x_root Cursor X coordinate in root window space.
 * @param cursor_y_root Cursor Y coordinate in root window space.
 * @return Bitmask combining RESIZE_LEFT, RESIZE_RIGHT, RESIZE_TOP, RESIZE_BOTTOM,
 *         or a corner constant (e.g., RESIZE_TOP_LEFT) for combined edges.
 *
 * Computes the cursor position relative to the window and checks if it lies
 * within RESIZE_BORDER pixels of any edge. Used during drag operations where
 * the window may have moved since the drag started.
 */
int detect_resize_edge_absolute(GtkWindow *window, int cursor_x_root, int cursor_y_root);

/**
 * Detect which resize edge or corner is near the cursor using local window coordinates.
 *
 * @param cursor_x      Cursor X coordinate relative to window.
 * @param cursor_y      Cursor Y coordinate relative to window.
 * @param window_width  Current window width.
 * @param window_height Current window height.
 * @return Bitmask combining RESIZE_LEFT, RESIZE_RIGHT, RESIZE_TOP, RESIZE_BOTTOM,
 *         or a corner constant for combined edges.
 *
 * Legacy version for use when cursor coordinates are already relative to the window.
 */
int detect_resize_edge(int cursor_x, int cursor_y, int window_width, int window_height);

/**
 * Update the window cursor based on the detected resize edge.
 *
 * @param widget      Widget whose window cursor will be changed.
 * @param resize_edge Resize edge bitmask (RESIZE_LEFT, RESIZE_TOP_RIGHT, etc.).
 *
 * Sets the appropriate cursor shape: ew-resize for left/right edges,
 * ns-resize for top/bottom, nwse-resize for top-left/bottom-right corners,
 * nesw-resize for top-right/bottom-left corners, or default for no resize.
 */
void update_resize_cursor(GtkWidget *widget, int resize_edge);

/**
 * Apply window resize based on the drag delta and resize edge.
 *
 * @param window         GTK window to resize.
 * @param resize_edge    Bitmask indicating which edges are being dragged.
 * @param delta_x        Horizontal mouse movement since drag start.
 * @param delta_y        Vertical mouse movement since drag start.
 * @param current_width  Current window width before resize.
 * @param current_height Current window height before resize.
 *
 * Updates window dimensions and position. When resizing from left or top edges,
 * the window position is adjusted to maintain the opposite edge fixed.
 * Enforces minimum size constraints (width >= 200, height >= 150) to prevent
 * the window from becoming unusable.
 */
void apply_window_resize(GtkWindow *window, int resize_edge,
                         int delta_x, int delta_y,
                         int current_width, int current_height);

#endif /* BLACKLINE_WINDOW_RESIZE_H */