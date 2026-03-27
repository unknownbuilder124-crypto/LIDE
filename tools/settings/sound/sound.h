#ifndef LIDE_SOUND_H
#define LIDE_SOUND_H

#include <gtk/gtk.h>

/**
 * Creates the sound settings tab.
 * Uses amixer commands to control system volume.
 *
 * @return GtkWidget containing the sound settings UI.
 */
GtkWidget *sound_tab_new(void);

#endif /* LIDE_SOUND_H */