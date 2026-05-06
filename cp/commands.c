#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * commands.c
 *
 * Built-in command implementations for Command Palette.
 * Provides app launching, file search, calculator, and system commands.
 */

/* Forward declarations for helper functions */
static char *evaluate_expression(const char *expr);
static void spawn_async(const char *cmd);

/**
 * Spawn a child process asynchronously.
 * Detaches from parent using setsid() so parent can exit.
 *
 * @param cmd Command string to execute
 */
static void spawn_async(const char *cmd)
{
    if (!cmd) return;

    pid_t pid = fork();
    if (pid == 0) {
        /* Child process */
        setsid();  /* Create new session */
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        perror("execl failed");
        exit(1);
    } else if (pid < 0) {
        perror("fork failed");
    }
}

/**
 * Simple expression evaluator.
 * Handles basic arithmetic: +, -, *, /, %
 * Allocates result string (caller must free).
 *
 * @param expr Expression to evaluate
 * @return Allocated result string
 */
static char *evaluate_expression(const char *expr)
{
    if (!expr) return NULL;

    static char result[256];

    /* Try to evaluate as a simple arithmetic expression */
    /* Use system bc command for robust evaluation */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "echo '%s' | bc -l", expr);

    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    if (fgets(result, sizeof(result), fp) != NULL) {
        /* Remove trailing newline */
        size_t len = strlen(result);
        if (len > 0 && result[len-1] == '\n') {
            result[len-1] = '\0';
        }
        pclose(fp);
        return result;
    }

    pclose(fp);
    return NULL;
}

/**
 * Launch an application.
 *
 * @param id   Command ID
 * @param data Application to launch (char*)
 */
void cmd_launch_app(const char *id, void *data)
{
    (void)id;
    const char *app = (const char *)data;
    if (!app) return;

    spawn_async(app);
}

/**
 * Search for files by name.
 * Uses find command with result output to file manager.
 *
 * @param id   Command ID
 * @param data Search pattern (char*)
 */
void cmd_search_files(const char *id, void *data)
{
    (void)id;
    const char *pattern = (const char *)data;
    if (!pattern) return;

    /* Use find with GUI notification */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "notify-send 'Searching for: %s' & "
             "find ~ -name '*%s*' -type f 2>/dev/null | head -20",
             pattern, pattern);

    spawn_async(cmd);
}

/**
 * Evaluate and calculate an expression.
 * Uses bc for mathematical operations.
 *
 * @param id   Command ID
 * @param data Expression to calculate (char*)
 */
void cmd_calculate(const char *id, void *data)
{
    (void)id;
    const char *expr = (const char *)data;
    if (!expr) return;

    char *result = evaluate_expression(expr);
    if (result) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "notify-send 'Calculation' '%s = %s'", expr, result);
        spawn_async(cmd);
    }
}

/**
 * Open system settings.
 *
 * @param id   Command ID
 * @param data Settings page (char*)
 */
void cmd_open_settings(const char *id, void *data)
{
    (void)id;
    const char *page = (const char *)data;
    if (!page) page = "";

    spawn_async("./blackline-settings");
}

/**
 * Execute system commands (shutdown, restart, etc).
 *
 * @param id   Command ID
 * @param data System action (char*)
 */
void cmd_system_action(const char *id, void *data)
{
    (void)id;
    const char *action = (const char *)data;
    if (!action) return;

    if (strcmp(action, "shutdown") == 0) {
        spawn_async("shutdown -h now");
    } else if (strcmp(action, "restart") == 0) {
        spawn_async("shutdown -r now");
    } else if (strcmp(action, "lock") == 0) {
        spawn_async("xlock");
    } else if (strcmp(action, "logout") == 0) {
        spawn_async("pkill -9 blackline-wm");
    }
}

/**
 * Register all built-in commands into the palette.
 *
 * @param state Palette state
 */
void commands_register_all(PaletteState *state)
{
    if (!state) return;

    /* Application launcher commands */
    palette_register_command(state, "app_terminal", "Terminal",
                           "Launch terminal", "utilities-terminal",
                           cmd_launch_app, (void*)"xterm");

    palette_register_command(state, "app_files", "File Manager",
                           "Open file manager", "system-file-manager",
                           cmd_launch_app, (void*)"./blackline-fm");

    palette_register_command(state, "app_editor", "Text Editor",
                           "Open text editor", "accessories-text-editor",
                           cmd_launch_app, (void*)"gedit");

    palette_register_command(state, "app_calculator", "Calculator",
                           "Open calculator", "accessories-calculator",
                           cmd_launch_app, (void*)"./blackline-calculator");

    palette_register_command(state, "app_monitor", "System Monitor",
                           "View system resources", "utilities-system-monitor",
                           cmd_launch_app, (void*)"./blackline-system-monitor");

    palette_register_command(state, "app_settings", "Settings",
                           "Open system settings", "preferences-system",
                           cmd_launch_app, (void*)"./blackline-settings");

    palette_register_command(state, "app_browser", "Web Browser",
                           "Open web browser", "internet-web-browser",
                           cmd_launch_app, (void*)"firefox");

    /* System commands */
    palette_register_command(state, "sys_shutdown", "Shutdown",
                           "Power off the system", "system-shutdown",
                           cmd_system_action, (void*)"shutdown");

    palette_register_command(state, "sys_restart", "Restart",
                           "Restart the system", "system-restart",
                           cmd_system_action, (void*)"restart");

    palette_register_command(state, "sys_lock", "Lock Screen",
                           "Lock the display", "system-lock-screen",
                           cmd_system_action, (void*)"lock");

    palette_register_command(state, "sys_logout", "Logout",
                           "Exit from desktop", "system-log-out",
                           cmd_system_action, (void*)"logout");

    /* Quick actions - placeholder (can be extended with dialog later) */
    palette_register_command(state, "qry_calculator", "Calculate",
                           "Evaluate expressions: 2+2, 5*3, etc", "accessories-calculator",
                           cmd_calculate, (void*)"");

    fprintf(stderr, "Registered %d built-in commands\n", 11);
}
