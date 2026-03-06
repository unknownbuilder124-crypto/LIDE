#!/bin/bash
cd ~/Desktop/LIDE

echo "Starting BlackLine..."

# Kill existing processes
pkill Xephyr 2>/dev/null
pkill blackline-background 2>/dev/null
pkill blackline-panel 2>/dev/null
pkill blackline-wm 2>/dev/null
sleep 1

# Get screen dimensions
SCREEN_WIDTH=$(xdpyinfo | awk '/dimensions/{print $2}' | cut -d'x' -f1)
SCREEN_HEIGHT=$(xdpyinfo | awk '/dimensions/{print $2}' | cut -d'x' -f2)

# Use 90% of screen size (with window decorations)
WIDTH=$((SCREEN_WIDTH * 9 / 10))
HEIGHT=$((SCREEN_HEIGHT * 9 / 10))

# Start Xephyr with large window (not fullscreen)
Xephyr :1 -screen ${WIDTH}x${HEIGHT} -ac &
XEPHYR_PID=$!
sleep 2

export DISPLAY=:1

# Start background (wallpaper)
./blackline-background &
BACKGROUND_PID=$!

# Start panel
./blackline-panel &
PANEL_PID=$!

# Start window manager
./blackline-wm &
WM_PID=$!

echo "BlackLine is running in ${WIDTH}x${HEIGHT} window!"
echo "Press Ctrl+Shift to release mouse/keyboard"
echo "Just close the Xephyr window to exit"
echo "Or press Ctrl+C in this terminal"

# Wait for Xephyr to exit (when window is closed)
wait $XEPHYR_PID

# Cleanup
kill $BACKGROUND_PID $PANEL_PID $WM_PID 2>/dev/null
echo "BlackLine stopped."