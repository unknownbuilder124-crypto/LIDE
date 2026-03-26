#!/bin/bash
cd ~/Desktop/LIDE

echo "Starting BlackLine..."

# Kill existing processes
pkill Xephyr 2>/dev/null
pkill blackline-background 2>/dev/null
pkill blackline-panel 2>/dev/null
pkill blackline-wm 2>/dev/null
sleep 1

# Find the actual paths to the binaries
WM_PATH=$(find . -name "blackline-wm" -type f | head -1)
PANEL_PATH=$(find . -name "blackline-panel" -type f | head -1)
BACKGROUND_PATH=$(find . -name "blackline-background" -type f | head -1)

if [ -z "$WM_PATH" ] || [ -z "$PANEL_PATH" ] || [ -z "$BACKGROUND_PATH" ]; then
    echo "Error: Could not find blackline binaries"
    echo "Please run 'make' first to build them"
    exit 1
fi

echo "Found binaries:"
echo "  WM: $WM_PATH"
echo "  Panel: $PANEL_PATH"
echo "  Background: $BACKGROUND_PATH"

# Get screen dimensions
SCREEN_WIDTH=$(xdpyinfo 2>/dev/null | awk '/dimensions/{print $2}' | cut -d'x' -f1)
SCREEN_HEIGHT=$(xdpyinfo 2>/dev/null | awk '/dimensions/{print $2}' | cut -d'x' -f2)

# Fallback if xdpyinfo fails
if [ -z "$SCREEN_WIDTH" ] || [ -z "$SCREEN_HEIGHT" ]; then
    SCREEN_WIDTH=1920
    SCREEN_HEIGHT=1080
fi

# Use 90% of screen size
WIDTH=$((SCREEN_WIDTH * 9 / 10))
HEIGHT=$((SCREEN_HEIGHT * 9 / 10))

# Ensure minimum size
if [ $WIDTH -lt 800 ]; then WIDTH=800; fi
if [ $HEIGHT -lt 600 ]; then HEIGHT=600; fi

# Function to find available display
find_available_display() {
    for ((i=5; i<=20; i++)); do
        if [ ! -e "/tmp/.X${i}-lock" ] && ! pgrep -f "Xephyr :${i}" > /dev/null; then
            echo $i
            return 0
        fi
    done
    echo -1
    return 1
}

# Find an available display
DISPLAY_NUM=$(find_available_display)

if [ "$DISPLAY_NUM" -eq -1 ]; then
    echo "Error: No available display found (tried :5 through :20)"
    echo "Cleaning up stale display locks..."
    for i in {5..20}; do
        sudo rm -f "/tmp/.X${i}-lock" 2>/dev/null
        sudo rm -f "/tmp/.X11-unix/X${i}" 2>/dev/null
    done
    DISPLAY_NUM=$(find_available_display)
    if [ "$DISPLAY_NUM" -eq -1 ]; then
        echo "Still no display available. Exiting."
        exit 1
    fi
fi

echo "Using display :${DISPLAY_NUM}"

# Clean up any leftover lock files for this display
sudo rm -f "/tmp/.X${DISPLAY_NUM}-lock" 2>/dev/null
sudo rm -f "/tmp/.X11-unix/X${DISPLAY_NUM}" 2>/dev/null

# Start Xephyr
Xephyr :${DISPLAY_NUM} -screen ${WIDTH}x${HEIGHT} -ac -resizeable &
XEPHYR_PID=$!
sleep 3

# Check if Xephyr started successfully
if ! kill -0 $XEPHYR_PID 2>/dev/null; then
    echo "Failed to start Xephyr on display :${DISPLAY_NUM}"
    exit 1
fi

export DISPLAY=:${DISPLAY_NUM}
export GDK_BACKEND=x11  # Force X11 backend
export PATH=.:$PATH  # Add current directory to PATH for easy access to binaries

# Verify X connection
echo "Verifying X connection..."
for i in {1..5}; do
    if xdpyinfo > /dev/null 2>&1; then
        echo "X server is responding"
        break
    fi
    if [ $i -eq 5 ]; then
        echo "ERROR: Cannot connect to X server"
        kill $XEPHYR_PID
        exit 1
    fi
    sleep 1
done

# Start window manager first (creates the container)
echo "Starting window manager..."
$WM_PATH &
WM_PID=$!
sleep 2

# Start panel next
echo "Starting panel..."
$PANEL_PATH &
PANEL_PID=$!
sleep 1

# Start background last
echo "Starting background..."
$BACKGROUND_PATH &
BACKGROUND_PID=$!

echo ""
echo "========================================="
echo "  BlackLine is running in ${WIDTH}x${HEIGHT} window on display :${DISPLAY_NUM}!"
echo "========================================="
echo "  Press Ctrl+Shift to release mouse/keyboard"
echo "  Just close the Xephyr window to exit"
echo "  Or press Ctrl+C in this terminal"
echo "========================================="
echo ""

# Wait for Xephyr to exit 
wait $XEPHYR_PID

# Cleanup
echo "Shutting down BlackLine..."
kill $BACKGROUND_PID $PANEL_PID $WM_PID 2>/dev/null
sudo rm -f "/tmp/.X${DISPLAY_NUM}-lock" 2>/dev/null
sudo rm -f "/tmp/.X11-unix/X${DISPLAY_NUM}" 2>/dev/null
echo "BlackLine stopped."