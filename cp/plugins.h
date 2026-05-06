#ifndef LIDE_PLUGINS_H
#define LIDE_PLUGINS_H

#include "command-palette.h"

/**
 * plugins.h
 *
 * Plugin system for Command Palette.
 * Defines the plugin interface and provides plugin management functions.
 */

/**
 * Plugin callback function.
 * Called when plugin is loaded to register its commands.
 *
 * @param state Palette state to register commands into
 */
typedef void (*PluginInit)(PaletteState *state);

/**
 * Plugin descriptor.
 *
 * @name       Plugin name
 * @version    Plugin version string
 * @author     Plugin author
 * @init_func  Initialization function
 */
typedef struct {
    const char *name;
    const char *version;
    const char *author;
    PluginInit init_func;
} Plugin;

/**
 * Load and initialize all built-in plugins.
 *
 * @param state Palette state to load plugins into
 */
void plugins_load_all(PaletteState *state);

/**
 * Register a plugin.
 *
 * @param plugin Plugin descriptor
 */
void plugins_register(const Plugin *plugin);

#endif /* LIDE_PLUGINS_H */
