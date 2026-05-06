#ifndef LIDE_COMMANDS_H
#define LIDE_COMMANDS_H

#include "command-palette.h"

/**
 * commands.h
 *
 * Built-in command implementations for Command Palette.
 * Provides app launching, file search, calculator, and system commands.
 */

/**
 * Register all built-in commands.
 *
 * @param state Palette state
 */
void commands_register_all(PaletteState *state);

/**
 * Launch/open an application by name or path.
 *
 * @param id    Command ID
 * @param data  Application name/path
 */
void cmd_launch_app(const char *id, void *data);

/**
 * Search for files matching a pattern.
 *
 * @param id    Command ID
 * @param data  Search pattern
 */
void cmd_search_files(const char *id, void *data);

/**
 * Evaluate a mathematical expression.
 *
 * @param id    Command ID
 * @param data  Expression to evaluate
 */
void cmd_calculate(const char *id, void *data);

/**
 * Open system settings.
 *
 * @param id    Command ID
 * @param data  Settings page/category
 */
void cmd_open_settings(const char *id, void *data);

/**
 * Execute system commands (shutdown, restart, etc).
 *
 * @param id    Command ID
 * @param data  System action
 */
void cmd_system_action(const char *id, void *data);

#endif /* LIDE_COMMANDS_H */
