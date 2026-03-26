#!/bin/bash

# BlackLine/LIDE Dependency Installer
# This script installs all required dependencies for building and running BlackLine

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect OS and distribution
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
        VERSION=$VERSION_ID
    elif [ -f /etc/debian_version ]; then
        OS="debian"
    elif [ -f /etc/arch-release ]; then
        OS="arch"
    elif [ -f /etc/fedora-release ]; then
        OS="fedora"
    elif [ "$(uname)" = "Darwin" ]; then
        OS="macos"
    else
        OS="unknown"
    fi
    print_status "Detected OS: $OS"
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        print_error "Please run as root (use sudo)"
        exit 1
    fi
}

# Update package manager
update_packages() {
    print_status "Updating package manager..."
    case $OS in
        debian|ubuntu|kali)
            apt-get update -y
            ;;
        fedora)
            dnf check-update -y || true
            ;;
        arch)
            pacman -Sy --noconfirm
            ;;
        *)
            print_warning "Cannot update packages for unknown OS"
            ;;
    esac
}

# Install dependencies for Debian/Ubuntu/Kali
install_debian() {
    print_status "Installing dependencies for Debian/Ubuntu/Kali..."
    
    # Base development tools
    apt-get install -y \
        build-essential \
        pkg-config \
        git \
        cmake \
        make \
        gcc \
        g++ \
        libc6-dev \
        autoconf \
        automake \
        libtool

    # X11 and window manager dependencies
    apt-get install -y \
        libx11-dev \
        libxft-dev \
        libxinerama-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxi-dev \
        libxext-dev \
        libxcomposite-dev \
        libxdamage-dev \
        libxfixes-dev \
        libxrender-dev \
        libxss-dev \
        libxtst-dev \
        x11proto-core-dev \
        x11proto-xinerama-dev \
        libxcb1-dev \
        libxcb-util0-dev \
        libxcb-icccm4-dev \
        libxcb-keysyms1-dev \
        libxcb-randr0-dev \
        libxcb-shape0-dev \
        libxcb-xinerama0-dev \
        libxcb-xkb-dev \
        libxcb-xrm-dev

    # GTK and GUI dependencies
    apt-get install -y \
        libgtk-3-dev \
        libgtk-4-dev \
        libgdk-pixbuf2.0-dev \
        libglib2.0-dev \
        libcairo2-dev \
        libpango1.0-dev \
        libatk1.0-dev \
        libgdk-pixbuf-2.0-dev \
        libharfbuzz-dev \
        libfontconfig1-dev \
        libfreetype6-dev \
        libepoxy-dev \
        libwayland-dev \
        libxkbcommon-dev \
        libegl1-mesa-dev \
        libgles2-mesa-dev

    # WebKit and browser dependencies
    apt-get install -y \
        libwebkit2gtk-4.1-dev \
        libwebkit2gtk-4.0-dev \
        libjavascriptcoregtk-4.1-dev \
        libsoup-3.0-dev \
        libsoup2.4-dev \
        libgcrypt20-dev \
        libsecret-1-dev

    # Image and graphics libraries
    apt-get install -y \
        libimlib2-dev \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        libgif-dev \
        libwebp-dev \
        librsvg2-dev \
        libopenjp2-7-dev \
        libjbig-dev \
        liblzma-dev

    # Terminal dependencies
    apt-get install -y \
        libvte-2.91-dev \
        libvte-2.91-gtk3-dev \
        libpcre2-dev

    # System monitoring
    apt-get install -y \
        libgtop2-dev \
        liblmdb-dev \
        libsystemd-dev \
        libprocps-dev \
        libsensors-dev \
        libudev-dev

    # NetworkManager dependencies
    apt-get install -y \
        libnm-dev \
        libnl-3-dev \
        libnl-genl-3-dev

    # Utilities
    apt-get install -y \
        xorg \
        xserver-xephyr \
        x11-utils \
        x11-xserver-utils \
        xinit \
        xterm \
        xdotool \
        wmctrl \
        x11vnc \
        xvfb

    # Additional tools
    apt-get install -y \
        curl \
        wget \
        nano \
        vim \
        htop \
        cmake \
        meson \
        ninja-build \
        valgrind \
        gdb \
        strace

    print_success "Debian/Ubuntu/Kali dependencies installed"
}

# Install dependencies for Fedora
install_fedora() {
    print_status "Installing dependencies for Fedora..."
    
    # Base development tools
    dnf install -y \
        gcc \
        gcc-c++ \
        make \
        cmake \
        git \
        pkgconfig \
        kernel-devel \
        autoconf \
        automake \
        libtool

    # X11 and window manager dependencies
    dnf install -y \
        libX11-devel \
        libXft-devel \
        libXinerama-devel \
        libXrandr-devel \
        libXcursor-devel \
        libXi-devel \
        libXext-devel \
        libXcomposite-devel \
        libXdamage-devel \
        libXfixes-devel \
        libXrender-devel \
        libXScrnSaver-devel \
        libXtst-devel \
        libxcb-devel \
        xcb-util-devel \
        xcb-util-keysyms-devel \
        xcb-util-wm-devel

    # GTK and GUI dependencies
    dnf install -y \
        gtk3-devel \
        gtk4-devel \
        gdk-pixbuf2-devel \
        glib2-devel \
        cairo-devel \
        pango-devel \
        atk-devel \
        harfbuzz-devel \
        fontconfig-devel \
        freetype-devel \
        libepoxy-devel \
        wayland-devel \
        libxkbcommon-devel

    # WebKit and browser dependencies
    dnf install -y \
        webkit2gtk4.1-devel \
        webkit2gtk4.0-devel \
        javascriptcoregtk4.1-devel \
        libsoup3-devel \
        libsoup-devel \
        libgcrypt-devel \
        libsecret-devel

    # Image and graphics libraries
    dnf install -y \
        imlib2-devel \
        libjpeg-turbo-devel \
        libpng-devel \
        libtiff-devel \
        giflib-devel \
        libwebp-devel \
        librsvg2-devel \
        openjpeg2-devel

    # Terminal dependencies
    dnf install -y \
        vte291-devel \
        vte-profile \
        pcre2-devel

    # System monitoring
    dnf install -y \
        libgtop2-devel \
        lmdb-devel \
        systemd-devel \
        procps-ng-devel \
        lm_sensors-devel \
        libudev-devel

    # NetworkManager dependencies
    dnf install -y \
        NetworkManager-libnm-devel \
        libnl3-devel

    # Utilities
    dnf install -y \
        xorg-x11-server-Xephyr \
        xorg-x11-utils \
        xorg-x11-xinit \
        xterm \
        xdotool \
        wmctrl \
        x11vnc \
        xorg-x11-server-Xvfb

    # Additional tools
    dnf install -y \
        curl \
        wget \
        nano \
        vim \
        htop \
        meson \
        ninja-build \
        valgrind \
        gdb \
        strace

    print_success "Fedora dependencies installed"
}

# Install dependencies for Arch Linux
install_arch() {
    print_status "Installing dependencies for Arch Linux..."
    
    # Base development tools
    pacman -S --noconfirm \
        base-devel \
        gcc \
        make \
        cmake \
        git \
        pkg-config \
        autoconf \
        automake \
        libtool

    # X11 and window manager dependencies
    pacman -S --noconfirm \
        libx11 \
        libxft \
        libxinerama \
        libxrandr \
        libxcursor \
        libxi \
        libxext \
        libxcomposite \
        libxdamage \
        libxfixes \
        libxrender \
        libxss \
        libxtst \
        xorgproto \
        libxcb \
        xcb-util \
        xcb-util-keysyms \
        xcb-util-wm

    # GTK and GUI dependencies
    pacman -S --noconfirm \
        gtk3 \
        gtk4 \
        gdk-pixbuf2 \
        glib2 \
        cairo \
        pango \
        atk \
        harfbuzz \
        fontconfig \
        freetype2 \
        libepoxy \
        wayland \
        libxkbcommon

    # WebKit and browser dependencies
    pacman -S --noconfirm \
        webkit2gtk \
        webkit2gtk-4.1 \
        javascriptcoregtk \
        libsoup3 \
        libsoup \
        libgcrypt \
        libsecret

    # Image and graphics libraries
    pacman -S --noconfirm \
        imlib2 \
        libjpeg-turbo \
        libpng \
        libtiff \
        giflib \
        libwebp \
        librsvg \
        openjpeg2

    # Terminal dependencies
    pacman -S --noconfirm \
        vte3 \
        vte-common \
        pcre2

    # System monitoring
    pacman -S --noconfirm \
        libgtop \
        lmdb \
        systemd-libs \
        procps-ng \
        lm_sensors \
        libudev0-shim

    # NetworkManager dependencies
    pacman -S --noconfirm \
        networkmanager \
        libnm \
        libnl

    # Utilities
    pacman -S --noconfirm \
        xorg-server-xephyr \
        xorg-xrandr \
        xorg-xinit \
        xterm \
        xdotool \
        wmctrl \
        x11vnc \
        xorg-server-xvfb

    # Additional tools
    pacman -S --noconfirm \
        curl \
        wget \
        nano \
        vim \
        htop \
        meson \
        ninja \
        valgrind \
        gdb \
        strace

    print_success "Arch Linux dependencies installed"
}

# Install dependencies for macOS (partial support)
install_macos() {
    print_warning "macOS support is experimental. Using Homebrew..."
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        print_error "Homebrew not found. Please install Homebrew first: https://brew.sh/"
        exit 1
    fi

    # Install dependencies
    brew install \
        pkg-config \
        gcc \
        make \
        cmake \
        git \
        autoconf \
        automake \
        libtool \
        libx11 \
        gtk+3 \
        gtk+4 \
        glib \
        cairo \
        pango \
        atk \
        harfbuzz \
        webkit2gtk \
        imlib2 \
        jpeg \
        libpng \
        libtiff \
        librsvg \
        vte \
        curl \
        wget \
        nano \
        vim \
        htop \
        meson \
        ninja \
        valgrind \
        gdb

    print_success "macOS dependencies installed (partial)"
}

# Install dependencies for unknown OS
install_unknown() {
    print_error "Unknown operating system. Please install dependencies manually."
    print_status "Required packages:"
    echo "  - X11 development libraries (libx11-dev, libxft-dev, libxinerama-dev, etc.)"
    echo "  - GTK3/GTK4 development libraries (libgtk-3-dev, libgtk-4-dev)"
    echo "  - WebKit2GTK development libraries (libwebkit2gtk-4.1-dev)"
    echo "  - Imlib2 development library (libimlib2-dev)"
    echo "  - VTE terminal library (libvte-2.91-dev)"
    echo "  - NetworkManager development (libnm-dev)"
    echo "  - System monitoring (libgtop2-dev, libprocps-dev)"
    echo "  - Build tools (gcc, make, cmake, pkg-config, autoconf, automake)"
    exit 1
}

# Verify installations
verify_installations() {
    print_status "Verifying critical installations..."
    
    # Check for critical headers/libraries
    check_pkg() {
        if pkg-config --exists "$1" 2>/dev/null; then
            print_success "Found $1 ($(pkg-config --modversion "$1"))"
            return 0
        else
            print_warning "Could not verify $1"
            return 1
        fi
    }
    
    check_header() {
        if [ -f "/usr/include/$1" ] || [ -f "/usr/local/include/$1" ]; then
            print_success "Found $1"
        else
            print_warning "Could not verify $1"
        fi
    }
    
    # Critical packages
    check_pkg "x11"
    check_pkg "gtk+-3.0"
    check_pkg "webkit2gtk-4.1" || check_pkg "webkit2gtk-4.0"
    check_pkg "glib-2.0"
    check_pkg "cairo"
    check_pkg "imlib2"
    check_pkg "vte-2.91" || check_pkg "vte-2.90"
    check_pkg "libnm" || print_warning "NetworkManager development library not found"
    check_pkg "libgtop-2.0" || print_warning "libgtop not found"
    
    # Check for executables
    check_cmd() {
        if command -v "$1" &> /dev/null; then
            print_success "Found $1"
        else
            print_warning "Could not find $1"
        fi
    }
    
    check_cmd "gcc"
    check_cmd "make"
    check_cmd "pkg-config"
    check_cmd "cmake"
    
    print_success "Verification complete"
}

# Main installation function
main() {
    echo "========================================="
    echo "  BlackLine/LIDE Dependency Installer"
    echo "========================================="
    
    # Check root
    check_root
    
    # Detect OS
    detect_os
    
    # Ask for confirmation
    echo
    print_warning "This will install development packages for your system."
    echo "The installation may take several minutes and use significant disk space."
    echo
    read -p "Continue? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_status "Installation cancelled."
        exit 0
    fi
    
    # Update packages
    update_packages
    
    # Install based on OS
    case $OS in
        debian|ubuntu|kali|raspbian)
            install_debian
            ;;
        fedora)
            install_fedora
            ;;
        arch)
            install_arch
            ;;
        macos)
            install_macos
            ;;
        *)
            install_unknown
            ;;
    esac
    
    # Verify installations
    verify_installations
    
    echo
    print_success "Dependency installation complete!"
    print_status "You can now build BlackLine/LIDE using 'make'"
    echo
    print_status "To run BlackLine: ./run.sh"
}

# Run main function
main "$@"