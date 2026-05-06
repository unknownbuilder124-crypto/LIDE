#ifndef LIDE_COMMAND_PALETTE_H
#define LIDE_COMMAND_PALETTE_H

#include <gtk/gtk.h>
#include <glib.h>

/**
 * command-palette.h
 *
 * Command Palette system - unified global overlay for launching apps,
 * running commands, searching files, calculating expressions.
 *
 * This module provides the main GTK application and filtering logic
 * for the Command Palette interface.
 */

/**
 * Represents a single command/action in the palette.
 *
 * @id          Unique identifier for the command
 * @display     Display name shown to user
 * @description Brief description/category
 * @icon        Icon name or path
 * @executor    Function to execute the command
 * @user_data   Context data for executor
 * @score       Search relevance score (calculated)
 */
typedef struct {
    char *id;
    char *display;
    char *description;
    char *icon;
    void (*executor)(const char *id, void *user_data);
    void *user_data;
    int score;
} PaletteCommand;

/**
 * Represents a search result row.
 * Stores the command and calculated match score.
 */
typedef struct {
    PaletteCommand *cmd;
    int match_score;
} SearchResult;

/**
 * Application state for the Command Palette.
 *
 * @window       Main GTK window
 * @listbox      GtkListBox for displaying results
 * @search_entry Search/filter input field
 * @commands     Array of all available commands
 * @num_commands Number of commands in array
 * @results      Filtered search results
 * @num_results  Number of current results
 */
typedef struct {
    GtkWidget *window;
    GtkWidget *listbox;
    GtkWidget *search_entry;
    PaletteCommand *commands;
    int num_commands;
    SearchResult *results;
    int num_results;
} PaletteState;

/**
 * Register a command in the palette.
 *
 * @param state    Palette state
 * @param id       Unique identifier
 * @param display  Display name
 * @param desc     Description/category
 * @param icon     Icon name
 * @param executor Function to call when executed
 * @param data     User data for executor
 */
void palette_register_command(
    PaletteState *state,
    const char *id,
    const char *display,
    const char *desc,
    const char *icon,
    void (*executor)(const char *id, void *user_data),
    void *data
);

/**
 * Execute a command by ID.
 *
 * @param state Palette state
 * @param id    Command ID to execute
 */
void palette_execute_command(PaletteState *state, const char *id);

/**
 * Get the application state (singleton).
 *
 * @return Pointer to global palette state
 */
PaletteState* palette_get_state(void);

#endif /* LIDE_COMMAND_PALETTE_H */
