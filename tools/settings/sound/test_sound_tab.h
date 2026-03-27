#ifndef LIDE_TEST_SOUND_TAB_H
#define LIDE_TEST_SOUND_TAB_H

#include <gtk/gtk.h>

/**
 * Creates the sound test visualization tab.
 * Provides visual feedback for sound testing with animated waveforms.
 *
 * @return GtkWidget containing sound test visualization UI
 */
GtkWidget *test_sound_tab_new(void);

#endif