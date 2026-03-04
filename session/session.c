#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

static pid_t wm_pid = 0;
static pid_t panel_pid = 0;
static pid_t wallpaper_pid = 0;

static void start_program(const char *path, pid_t *pid) 

{
    *pid = fork();
    if (*pid == 0) {
        execl(path, path, NULL);
        perror("execl failed");
        exit(1);
    }
}

static void cleanup(int sig) 

{
    (void)sig;
    if (wm_pid > 0) kill(wm_pid, SIGTERM);
    if (panel_pid > 0) kill(panel_pid, SIGTERM);
    if (wallpaper_pid > 0) kill(wallpaper_pid, SIGTERM);
    exit(0);
}


int main(void) 

{
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    start_program("./blackline-wm", &wm_pid);
    if (wm_pid < 0) return 1;

    sleep(1);

    start_program("./blackline-wallpaper", &wallpaper_pid);

    start_program("./blackline-panel", &panel_pid);
    if (panel_pid < 0) 
    {
        kill(wm_pid, SIGTERM);
        return 1;
    }

    int status;

    pid_t exited = wait(&status);

    if (exited == wm_pid || exited == panel_pid) 
    {
        if (wm_pid > 0 && exited != wm_pid) kill(wm_pid, SIGTERM);
        if (panel_pid > 0 && exited != panel_pid) kill(panel_pid, SIGTERM);
        if (wallpaper_pid > 0) kill(wallpaper_pid, SIGTERM);
    }

    return 0;
}