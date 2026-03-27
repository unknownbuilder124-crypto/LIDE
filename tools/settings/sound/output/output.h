#ifndef LIDE_SOUND_OUTPUT_H
#define LIDE_SOUND_OUTPUT_H

#include <gtk/gtk.h>

/**
 * Creates the output volume control widget.
 * Displays a scale for master volume and a balance control.
 *
 * @return GtkWidget containing output volume controls.
 */
GtkWidget *output_volume_widget_new(void);

/**
 * Updates the output volume display with current PulseAudio settings.
 * Should be called when the volume changes externally.
 */
void output_volume_refresh(void);

#endif /* LIDE_SOUND_OUTPUT_H */