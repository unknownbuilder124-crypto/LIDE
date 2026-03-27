#include "recycle_bin.h"
#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>

/**
 * Checks if the given path is the user's trash directory.
 */
gboolean is_trash_directory(const gchar *path)
{
    if (!path) return FALSE;
    
    const gchar *home = g_get_home_dir();
    gchar *trash_path = g_build_filename(home, ".local/share/Trash/files", NULL);
    
    gboolean result = (g_strcmp0(path, trash_path) == 0);
    
    g_free(trash_path);
    return result;
}

/**
 * Helper to recursively delete a directory and its contents.
 * Used for emptying trash.
 */
static void delete_directory_contents(const gchar *dir_path)
{
    GDir *dir = g_dir_open(dir_path, 0, NULL);
    if (!dir) return;
    
    const gchar *entry;
    while ((entry = g_dir_read_name(dir)) != NULL) {
        gchar *full_path = g_build_filename(dir_path, entry, NULL);
        GFile *file = g_file_new_for_path(full_path);
        g_file_delete(file, NULL, NULL);
        g_object_unref(file);
        g_free(full_path);
    }
    
    g_dir_close(dir);
}

/**
 * Permanently empties the trash.
 */
gboolean empty_trash(void)
{
    const gchar *home = g_get_home_dir();
    gchar *trash_files = g_build_filename(home, ".local/share/Trash/files", NULL);
    gchar *trash_info = g_build_filename(home, ".local/share/Trash/info", NULL);
    
    /* Delete all files in trash/files */
    delete_directory_contents(trash_files);
    
    /* Delete all .trashinfo files in trash/info */
    delete_directory_contents(trash_info);
    
    g_free(trash_files);
    g_free(trash_info);
    
    return TRUE;
}
