#ifndef LIDE_RECYCLE_BIN_H
#define LIDE_RECYCLE_BIN_H

#include <glib.h>

/**
 * Checks if the given path is the user's trash directory.
 *
 * @param path The path to check.
 * @return TRUE if path matches the trash directory, FALSE otherwise.
 */
gboolean is_trash_directory(const gchar *path);

/**
 * Permanently deletes all files in the trash directory.
 * Removes both files and their associated metadata.
 *
 * @sideeffect Deletes all files in ~/.local/share/Trash/files/ and corresponding info.
 * @return TRUE on success, FALSE on error.
 */
gboolean empty_trash(void);

#endif /* LIDE_RECYCLE_BIN_H */