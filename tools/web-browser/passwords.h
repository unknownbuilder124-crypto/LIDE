#ifndef VOIDFOX_PASSWORDS_H
#define VOIDFOX_PASSWORDS_H

#include <gtk/gtk.h>
#include "voidfox.h"

// Password entry structure
typedef struct {
    char *site;
    char *username;
    char *password;
} PasswordEntry;

// Function prototypes
void show_passwords_tab(BrowserWindow *browser);
void add_password(const char *site, const char *username, const char *password);
void save_passwords(void);
void load_passwords(void);

#endif