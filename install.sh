#!/bin/bash

# Installation script for LXQt Weather Widget
# This script builds and installs the weather plugin for LXQt panel

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root for installation
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_error "Do not run this script as root for building. It will ask for sudo when needed."
        exit 1
    fi
}

# Check system dependencies
check_dependencies() {
    print_status "Checking system dependencies..."

    local missing_deps=()

    # Check for required tools
    command -v gcc >/dev/null 2>&1 || missing_deps+=("gcc")
    command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")
    command -v cmake >/dev/null 2>&1 || missing_deps+=("cmake")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")

    # Check GCC version
    if command -v gcc >/dev/null 2>&1; then
        gcc_version=$(gcc -dumpversion | cut -d. -f1)
        if [ "$gcc_version" -lt 12 ]; then
            print_error "GCC version 12 or higher is required (found: $gcc_version)"
            exit 1
        fi
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_status "Please install them using your package manager:"

        # Detect package manager and suggest installation command
        if command -v apt >/dev/null 2>&1; then
            echo "  sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev liblxqt2-dev lxqt2-build-tools"
        elif command -v pacman >/dev/null 2>&1; then
            echo "  sudo pacman -S gcc cmake qt6-base qt6-tools lxqt2-build-tools lxqt"
        elif command -v dnf >/dev/null 2>&1; then
            echo "  sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qttools-devel lxqt-build-tools lxqt-panel-devel"
        else
            echo "  Install: gcc, g++, cmake, make, qt6-dev, lxqt-dev packages"
        fi

        exit 1
    fi

    print_status "All dependencies are satisfied"
}

# Build the project
build_project() {
    print_status "Building LXQt Weather Widget..."

    # Create build directory
    if [ -d "build" ]; then
        print_warning "Build directory exists, cleaning..."
        rm -rf build
    fi

    mkdir build
    cd build

    # Configure
    print_status "Configuring project..."
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release

    # Build
    print_status "Compiling..."
    make -j$(nproc)

    print_status "Build completed successfully"
}

# Install the plugin
install_plugin() {
    print_status "Installing LXQt Weather Widget..."

    if [ ! -d "build" ]; then
        print_error "Build directory not found. Please run build first."
        exit 1
    fi

    cd build

    # Install
    print_status "Installing plugin (may require sudo password)..."
    sudo make install

    print_status "Installation completed"
}

# Restart LXQt panel
restart_panel() {
    print_status "Restarting LXQt panel..."

    if pgrep -x "lxqt-panel" >/dev/null; then
        print_status "Stopping current panel..."
        killall lxqt-panel 2>/dev/null || true
        sleep 2
    fi

    print_status "Starting LXQt panel..."
    lxqt-panel &

    print_status "Panel restarted"
}

# Show configuration instructions
show_config_instructions() {
    echo
    print_status "Installation completed successfully!"
    echo
    echo "Next steps:"
    echo "1. Get a free API key from https://openweathermap.org/api"
    echo "2. Right-click on the LXQt panel"
    echo "3. Select 'Configure Panel' → 'Widgets'"
    echo "4. Add the 'Weather' widget"
    echo "5. Right-click on the weather widget and select 'Configure Weather'"
    echo "6. Enter your OpenWeatherMap API key"
    echo "7. Configure other settings as desired"
    echo
    print_warning "Note: It may take up to 2 hours for a new API key to become active"
}

# Main function
main() {
    local action="${1:-all}"

    case "$action" in
        "check")
            check_root
            check_dependencies
            ;;
        "build")
            check_root
            check_dependencies
            build_project
            ;;
        "install")
            install_plugin
            ;;
        "restart")
            restart_panel
            ;;
        "all")
            check_root
            check_dependencies
            build_project
            install_plugin
            restart_panel
            show_config_instructions
            ;;
        *)
            echo "Usage: $0 [check|build|install|restart|all]"
            echo
            echo "Commands:"
            echo "  check     - Check system dependencies"
            echo "  build     - Build the project"
            echo "  install   - Install the plugin"
            echo "  restart   - Restart LXQt panel"
            echo "  all       - Do everything (default)"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
