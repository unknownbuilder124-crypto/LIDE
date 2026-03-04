#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() 

{
    const char *wallpaper = "./LIDE/images/wal1.png";
    
    // Check if file exists
    if (access(wallpaper, F_OK) == 0) 
    {
        // Try feh
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "feh --bg-scale '%s'", wallpaper);
        system(cmd);
        
        // Also set with xsetroot as fallback
        system("xsetroot -solid '#0b0f14'");
        
        printf("Wallpaper set\n");
    } else {
        // Fallback to solid color
        system("xsetroot -solid '#0b0f14'");
        printf("Wallpaper not found, using solid color\n");
    }
    
    return 0;
}