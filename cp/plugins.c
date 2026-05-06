#include "plugins.h"
#include "commands.h"
#include <stdio.h>
#include <string.h>

/**
 * plugins.c
 *
 * Plugin system implementation for Command Palette.
 * Manages plugin registration and initialization.
 */

#define MAX_PLUGINS 32

static Plugin *registered_plugins[MAX_PLUGINS];
static int num_plugins = 0;

/**
 * Register a plugin with the system.
 *
 * @param plugin Plugin descriptor to register
 */
void plugins_register(const Plugin *plugin)
{
    if (!plugin || num_plugins >= MAX_PLUGINS) {
        fprintf(stderr, "Warning: Could not register plugin (max reached or invalid)\n");
        return;
    }

    registered_plugins[num_plugins] = (Plugin *)plugin;
    num_plugins++;
    fprintf(stderr, "Registered plugin: %s v%s by %s\n",
            plugin->name, plugin->version, plugin->author);
}

/**
 * Load and initialize all built-in plugins.
 */
void plugins_load_all(PaletteState *state)
{
    if (!state) return;

    /* Load built-in commands as core plugin */
    commands_register_all(state);

    /* Initialize all registered plugins */
    for (int i = 0; i < num_plugins; i++) {
        if (registered_plugins[i] && registered_plugins[i]->init_func) {
            registered_plugins[i]->init_func(state);
        }
    }
}
