#ifndef FILE_CHOOSER_H
#define FILE_CHOOSER_H

#include <gtk/gtk.h>

/**
 * File chooser modes
 */
typedef enum {
    CHOOSER_FILE,      /**< Select/Create a file */
    CHOOSER_FOLDER     /**< Select/Create a folder */
} FileChooserMode;

/**
 * File chooser actions
 */
typedef enum {
    CHOOSER_ACTION_CREATE,  /**< Create a new file/folder */
    CHOOSER_ACTION_SAVE,    /**< Save an existing file (shows overwrite warning) */
    CHOOSER_ACTION_OPEN     /**< Open an existing file (select mode) */
} FileChooserAction;

/**
 * FileChooser structure containing all UI elements and state
 */
typedef struct {
    GtkWidget *dialog;           /**< Main dialog window */
    GtkWidget *home_button;      /**< Home directory navigation button */
    GtkWidget *up_button;        /**< Parent directory navigation button */
    GtkWidget *location_entry;   /**< Current path display (read-only) */
    GtkWidget *search_entry;     /**< Search/filter entry field */
    GtkWidget *file_list;        /**< GtkTreeView displaying files/directories */
    GtkListStore *file_store;    /**< Data store for file list */
    GtkWidget *filename_entry;   /**< User input for filename/foldername */
    
    char current_path[1024];     /**< Currently browsed directory path */
    char selected_path[1024];    /**< Path selected by user (or empty if cancelled) */
    int completed;               /**< Flag indicating dialog completion */
    FileChooserMode mode;        /**< Current mode (FILE or FOLDER) */
    FileChooserAction action;    /**< Current action (CREATE or SAVE) */
} FileChooser;

/**
 * Create and initialize a new file chooser dialog.
 *
 * @param mode           CHOOSER_FILE or CHOOSER_FOLDER
 * @param action         CHOOSER_ACTION_CREATE or CHOOSER_ACTION_SAVE
 * @param initial_path   Starting directory (NULL = user's home directory)
 * @return               Newly allocated FileChooser structure
 */
FileChooser* file_chooser_new(FileChooserMode mode, FileChooserAction action, const char *initial_path);

/**
 * Display dialog modally and return user's selection.
 *
 * @param chooser        FileChooser instance
 * @return               Allocated string with selected path, or NULL if cancelled
 */
char* file_chooser_show(FileChooser *chooser);

/**
 * Clean up file chooser and free all resources.
 *
 * @param chooser        FileChooser instance
 */
void file_chooser_destroy(FileChooser *chooser);

#endif /* FILE_CHOOSER_H */