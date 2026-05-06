#include "command-palette.h"
#include "plugins.h"
#include "commands.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/**
 * command-palette.c
 *
 * Main Command Palette GTK application.
 * Provides the searchable overlay for commands, with real-time filtering,
 * command execution, and plugin integration.
 *
 * Architecture: Single-window GTK application with GtkListBox for commands.
 * Filters commands in real-time as user types. Executes selected command
 * on Enter key press.
 */

/* Global palette state (singleton) */
static PaletteState global_state = {0};

/* Command capacity and allocation */
#define INITIAL_CAPACITY 128
#define REALLOC_CHUNK 64
#define MAX_RESULTS 256

/**
 * Scoring function for command filtering.
 * Higher score = better match.
 *
 * Rules:
 * 1. Prefix match (display starts with query) = base score
 * 2. Contained match (query in display) = base * 0.7
 * 3. Contained in description = base * 0.4
 * 4. Bonus: exact case match +10
 *
 * @param cmd   Command to score
 * @param query Search query
 * @return      Match score (0 if no match)
 */
static int score_command(const PaletteCommand *cmd, const char *query)
{
    if (!cmd || !query || !*query) return 0;

    int base_score = 100;
    int score = 0;

    /* Exact case-insensitive match in display name */
    if (strcasestr(cmd->display, query)) {
        score = base_score;

        /* Prefix match bonus */
        if (strncasecmp(cmd->display, query, strlen(query)) == 0) {
            score = base_score + 50;
        }

        /* Exact case bonus */
        if (strstr(cmd->display, query)) {
            score += 10;
        }
    }

    /* Description match (lower priority) */
    if (!score && cmd->description && strcasestr(cmd->description, query)) {
        score = base_score * 0.4;
    }

    /* ID match (lowest priority) */
    if (!score && strcasestr(cmd->id, query)) {
        score = base_score * 0.3;
    }

    return score;
}

/**
 * Comparison function for sort (highest score first).
 */
static int compare_results(const void *a, const void *b)
{
    const SearchResult *ra = (const SearchResult *)a;
    const SearchResult *rb = (const SearchResult *)b;
    return rb->match_score - ra->match_score;
}

/**
 * Filter and search commands based on query.
 * Updates results and returns count.
 *
 * @param state Palette state
 * @param query Search query string
 * @return      Number of results
 */
static int filter_commands(PaletteState *state, const char *query)
{
    if (!state) return 0;

    state->num_results = 0;

    /* If no query, show all commands */
    if (!query || !*query) {
        for (int i = 0; i < state->num_commands; i++) {
            if (state->num_results >= MAX_RESULTS) break;
            state->results[state->num_results].cmd = &state->commands[i];
            state->results[state->num_results].match_score = 1;
            state->num_results++;
        }
    } else {
        /* Score each command and keep matches */
        for (int i = 0; i < state->num_commands; i++) {
            int score = score_command(&state->commands[i], query);
            if (score > 0) {
                if (state->num_results >= MAX_RESULTS) break;
                state->results[state->num_results].cmd = &state->commands[i];
                state->results[state->num_results].match_score = score;
                state->num_results++;
            }
        }
    }

    /* Sort by score (highest first) */
    qsort(state->results, state->num_results, sizeof(SearchResult), compare_results);

    return state->num_results;
}

/**
 * Update the list box with filtered results.
 *
 * @param state Palette state
 */
static void update_listbox(PaletteState *state)
{
    if (!state || !state->listbox) return;

    /* Clear existing rows */
    GList *children = gtk_container_get_children(GTK_CONTAINER(state->listbox));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);

    /* Add filtered results as rows */
    for (int i = 0; i < state->num_results; i++) {
        PaletteCommand *cmd = state->results[i].cmd;

        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_container_add(GTK_CONTAINER(row), box);

        /* Command name (bold) */
        GtkWidget *label_name = gtk_label_new(cmd->display);
        gtk_label_set_markup(GTK_LABEL(label_name),
                            g_strdup_printf("<b>%s</b>", cmd->display));
        gtk_label_set_xalign(GTK_LABEL(label_name), 0);
        gtk_box_pack_start(GTK_BOX(box), label_name, FALSE, FALSE, 0);

        /* Command description (gray, smaller) */
        if (cmd->description) {
            GtkWidget *label_desc = gtk_label_new(cmd->description);
            gtk_label_set_markup(GTK_LABEL(label_desc),
                                g_strdup_printf("<small><i>%s</i></small>", cmd->description));
            gtk_label_set_xalign(GTK_LABEL(label_desc), 0);
            gtk_box_pack_start(GTK_BOX(box), label_desc, FALSE, FALSE, 0);
        }

        /* Store command pointer for execution */
        g_object_set_data_full(G_OBJECT(row), "cmd", cmd, NULL);

        gtk_list_box_insert(GTK_LIST_BOX(state->listbox), row, -1);
    }

    gtk_widget_show_all(state->listbox);

    /* Auto-select first result */
    GtkListBoxRow *first = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->listbox), 0);
    if (first) {
        gtk_list_box_select_row(GTK_LIST_BOX(state->listbox), first);
    }
}

/**
 * Callback when search text changes.
 * Filters commands and updates the list box.
 *
 * @param entry Search entry widget
 * @param data  User data (PaletteState*)
 */
static void search_changed(GtkSearchEntry *entry, gpointer data)
{
    PaletteState *state = (PaletteState *)data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(entry));

    filter_commands(state, query);
    update_listbox(state);
}

/**
 * Callback when a row is activated (Enter key or button press).
 * Executes the selected command.
 *
 * @param listbox GtkListBox widget
 * @param row     Activated row
 * @param data    User data (PaletteState*)
 */
static void on_row_activated(GtkListBox *listbox, GtkListBoxRow *row, gpointer data)
{
    (void)listbox;
    PaletteState *state = (PaletteState *)data;

    if (!row) return;

    PaletteCommand *cmd = (PaletteCommand *)g_object_get_data(G_OBJECT(row), "cmd");
    if (!cmd || !cmd->executor) return;

    /* Execute the command */
    cmd->executor(cmd->id, cmd->user_data);

    /* Close the palette */
    gtk_window_close(GTK_WINDOW(state->window));
}

/**
 * Key press event handler.
 * Handles Escape to close, Up/Down for navigation.
 *
 * @param widget  Event widget
 * @param event   Key event
 * @param data    User data (PaletteState*)
 * @return        TRUE if handled, FALSE to propagate
 */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    (void)widget;
    PaletteState *state = (PaletteState *)data;

    if (event->keyval == GDK_KEY_Escape) {
        gtk_window_close(GTK_WINDOW(state->window));
        return TRUE;
    }

    return FALSE;
}

/**
 * Toggle function for testing - shows/hides all results.
 * Can be used for demo mode.
 */
void palette_show_all(PaletteState *state)
{
    if (!state) return;
    filter_commands(state, "");
    update_listbox(state);
}

/**
 * Application activation callback.
 * Creates and displays the main window UI.
 *
 * @param app       GtkApplication
 * @param user_data User data (unused)
 */
static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;
    PaletteState *state = &global_state;

    /* Create main window */
    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "Command Palette");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 600, 400);
    gtk_window_set_type_hint(GTK_WINDOW(state->window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(state->window), TRUE);

    /* Create layout */
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(state->window), box);

    /* Search entry with placeholder */
    state->search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->search_entry),
                                   "Type to search commands (Esc to cancel)...");
    gtk_box_pack_start(GTK_BOX(box), state->search_entry, FALSE, FALSE, 0);
    g_signal_connect(state->search_entry, "search-changed", G_CALLBACK(search_changed), state);

    /* Scrolled window for list */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);

    /* List box for commands */
    state->listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state->listbox),
                                    GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(scrolled), state->listbox);
    g_signal_connect(state->listbox, "row-activated",
                    G_CALLBACK(on_row_activated), state);

    /* Key press events */
    g_signal_connect(state->window, "key-press-event",
                    G_CALLBACK(on_key_press), state);

    /* Load and register all commands */
    plugins_load_all(state);

    /* Show initial results */
    update_listbox(state);

    /* Show all widgets */
    gtk_widget_show_all(state->window);

    /* Focus on search entry */
    gtk_widget_grab_focus(state->search_entry);
}

/**
 * Register a command in the palette.
 *
 * @param state    Palette state
 * @param id       Unique identifier
 * @param display  Display name
 * @param desc     Description
 * @param icon     Icon name
 * @param executor Function to execute
 * @param data     User data
 */
void palette_register_command(
    PaletteState *state,
    const char *id,
    const char *display,
    const char *desc,
    const char *icon,
    void (*executor)(const char *id, void *user_data),
    void *data)
{
    if (!state || !id || !display || !executor) return;

    /* Expand commands array if needed */
    static int capacity = 0;
    if (capacity == 0) {
        state->commands = malloc(INITIAL_CAPACITY * sizeof(PaletteCommand));
        capacity = INITIAL_CAPACITY;
    } else if (state->num_commands >= capacity) {
        capacity += REALLOC_CHUNK;
        state->commands = realloc(state->commands, capacity * sizeof(PaletteCommand));
    }

    /* Add command */
    PaletteCommand *cmd = &state->commands[state->num_commands++];
    cmd->id = strdup(id);
    cmd->display = strdup(display);
    cmd->description = desc ? strdup(desc) : NULL;
    cmd->icon = icon ? strdup(icon) : NULL;
    cmd->executor = executor;
    cmd->user_data = data;
    cmd->score = 0;

    fprintf(stderr, "Registered command: %s (%s)\n", display, id);
}

/**
 * Execute a command by ID.
 *
 * @param state Palette state
 * @param id    Command ID
 */
void palette_execute_command(PaletteState *state, const char *id)
{
    if (!state || !id) return;

    for (int i = 0; i < state->num_commands; i++) {
        if (strcmp(state->commands[i].id, id) == 0) {
            if (state->commands[i].executor) {
                state->commands[i].executor(id, state->commands[i].user_data);
            }
            return;
        }
    }
}

/**
 * Get the global palette state.
 *
 * @return Pointer to global state
 */
PaletteState* palette_get_state(void)
{
    return &global_state;
}

/**
 * Application entry point.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return     Exit status
 */
int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("org.blackline.command-palette",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    /* Allocate result buffer */
    global_state.results = malloc(MAX_RESULTS * sizeof(SearchResult));

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}
