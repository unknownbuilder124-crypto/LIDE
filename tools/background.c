#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int running = 1;

void handle_signal(int sig) 

{
    running = 0;
}

int main() 

{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    const char *wallpaper = "./images/wal1.png";
    
    printf("Background service started\n");
    
    while (running) {
        // Set wallpaper
        if (access(wallpaper, F_OK) == 0) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "feh --bg-scale '%s' 2>/dev/null", wallpaper);
            system(cmd);
        } else {
            system("xsetroot -solid '#0b0f14' 2>/dev/null");
        }
        
        // Set every 2 seconds to fight window manager
        sleep(2);
    }
    
    return 0;
}