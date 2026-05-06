#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


/*
 * session.c
 * 
 * X11 session management and environment initialization
Loads user preferences, initializes X11 server connection, manages
window manager process lifecycle, and handles graceful shutdown.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

static pid_t wm_pid = 0;    /* Process ID of the window manager process */
static pid_t panel_pid = 0; /* Process ID of the panel process */

/**
 * Forks and executes a program.
 * On success, the child process replaces itself with the target program.
 *
 * @param path Full path to the executable to run.
 * @param pid  Output parameter that receives the child process ID.
 *
 * @sideeffect Forks a new process. Child process execs the target program.
 *             On failure, prints error message and exits the child.
 */
static void start_program(const char *path, pid_t *pid) 
{
    *pid = fork();
    if (*pid == 0) {
        execl(path, path, NULL);
        perror("execl failed");
        exit(1);
    }
}

/**
 * Signal handler for SIGINT and SIGTERM.
 * Terminates both the window manager and panel processes before exiting.
 *
 * @param sig Signal number (unused).
 *
 * @sideeffect Sends SIGTERM to both child processes, then exits.
 */
static void cleanup(int sig) 
{
    (void)sig;
    if (wm_pid > 0) kill(wm_pid, SIGTERM);
    if (panel_pid > 0) kill(panel_pid, SIGTERM);
    exit(0);
}

/**
 * Session manager entry point.
 * Launches the window manager and panel as child processes,
 * waits for either to exit, then terminates the other.
 *
 * @return 0 on successful shutdown, 1 on startup failure.
 *
 * @sideeffect Forks child processes for WM and panel.
 * @sideeffect Installs signal handlers for clean shutdown.
 * @sideeffect Blocks waiting for child processes.
 */
int main(void) 
{
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    start_program("./blackline-wm", &wm_pid);
    if (wm_pid < 0) return 1;

    /* Allow window manager to initialize before launching panel */
    sleep(1);

    start_program("./blackline-panel", &panel_pid);
    if (panel_pid < 0) 
    {
        kill(wm_pid, SIGTERM);
        return 1;
    }

    int status;
    pid_t exited = wait(&status);

    /* If either child exits, terminate the other */
    if (exited == wm_pid || exited == panel_pid) 
    {
        if (wm_pid > 0 && exited != wm_pid) kill(wm_pid, SIGTERM);
        if (panel_pid > 0 && exited != panel_pid) kill(panel_pid, SIGTERM);
    }

    return 0;
}