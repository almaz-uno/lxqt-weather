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
    local missing_dev_packages=()

    # Check for required tools
    command -v gcc >/dev/null 2>&1 || missing_deps+=("gcc")
    command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")
    command -v cmake >/dev/null 2>&1 || missing_deps+=("cmake")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")
    command -v pkg-config >/dev/null 2>&1 || missing_deps+=("pkg-config")

    # Check GCC version
    if command -v gcc >/dev/null 2>&1; then
        gcc_version=$(gcc -dumpversion | cut -d. -f1)
        if [ "$gcc_version" -lt 12 ]; then
            print_error "GCC version 12 or higher is required (found: $gcc_version)"
            exit 1
        fi
    fi

    # Check for Qt5 development packages
    if ! pkg-config --exists Qt5Core Qt5Widgets Qt5Network 2>/dev/null; then
        missing_dev_packages+=("qtbase5-dev")
    fi

    if ! pkg-config --exists Qt5X11Extras 2>/dev/null; then
        missing_dev_packages+=("libqt5x11extras5-dev")
    fi

    # Check for LXQt development packages
    if ! pkg-config --exists lxqt 2>/dev/null; then
        missing_dev_packages+=("liblxqt1-dev")
    fi

    # Install missing dependencies automatically
    if [ ${#missing_deps[@]} -ne 0 ] || [ ${#missing_dev_packages[@]} -ne 0 ]; then
        if [ ${#missing_deps[@]} -ne 0 ]; then
            print_error "Missing build tools: ${missing_deps[*]}"
        fi
        if [ ${#missing_dev_packages[@]} -ne 0 ]; then
            print_error "Missing development packages: ${missing_dev_packages[*]}"
        fi

        print_status "Attempting to install missing dependencies..."

        # Detect package manager and install dependencies
        if command -v apt >/dev/null 2>&1; then
            local all_packages=()

            # Add build tools if missing
            if [ ${#missing_deps[@]} -ne 0 ]; then
                all_packages+=("build-essential" "cmake" "pkg-config")
            fi

            # Add development packages
            all_packages+=("${missing_dev_packages[@]}")

            print_status "Installing packages: ${all_packages[*]}"
            sudo apt update && sudo apt install -y "${all_packages[@]}"

        elif command -v pacman >/dev/null 2>&1; then
            local all_packages=()

            if [ ${#missing_deps[@]} -ne 0 ]; then
                all_packages+=("base-devel" "cmake" "pkgconf")
            fi

            # Map Debian package names to Arch package names
            for pkg in "${missing_dev_packages[@]}"; do
                case "$pkg" in
                    "qtbase5-dev") all_packages+=("qt5-base") ;;
                    "libqt5x11extras5-dev") all_packages+=("qt5-x11extras") ;;
                    "liblxqt1-dev") all_packages+=("lxqt-build-tools" "lxqt-panel") ;;
                esac
            done

            print_status "Installing packages: ${all_packages[*]}"
            sudo pacman -S --noconfirm "${all_packages[@]}"

        elif command -v dnf >/dev/null 2>&1; then
            local all_packages=()

            if [ ${#missing_deps[@]} -ne 0 ]; then
                all_packages+=("gcc-c++" "cmake" "make" "pkgconfig")
            fi

            # Map Debian package names to Fedora package names
            for pkg in "${missing_dev_packages[@]}"; do
                case "$pkg" in
                    "qtbase5-dev") all_packages+=("qt5-qtbase-devel") ;;
                    "libqt5x11extras5-dev") all_packages+=("qt5-qtx11extras-devel") ;;
                    "liblxqt1-dev") all_packages+=("lxqt-build-tools" "lxqt-panel-devel") ;;
                esac
            done

            print_status "Installing packages: ${all_packages[*]}"
            sudo dnf install -y "${all_packages[@]}"

        else
            print_error "Unknown package manager. Please install the following manually:"
            echo "Build tools: gcc, g++, cmake, make, pkg-config"
            echo "Qt5 development: qtbase5-dev, libqt5x11extras5-dev"
            echo "LXQt development: liblxqt1-dev, lxqt-build-tools"
            exit 1
        fi

        # Re-check dependencies after installation
        print_status "Verifying installation..."
        if ! pkg-config --exists Qt5Core Qt5Widgets Qt5Network Qt5X11Extras lxqt 2>/dev/null; then
            print_error "Some dependencies are still missing after installation. Please check the error messages above."
            exit 1
        fi
    fi

    print_status "All dependencies are satisfied"

    # Show brief summary of what's installed
    if command -v pkg-config >/dev/null 2>&1; then
        if pkg-config --exists Qt5Core Qt5Widgets Qt5Network Qt5X11Extras lxqt 2>/dev/null; then
            qt_version=$(pkg-config --modversion Qt5Core 2>/dev/null || echo "unknown")
            lxqt_version=$(pkg-config --modversion lxqt 2>/dev/null || echo "unknown")
            print_status "Found Qt5 version: $qt_version"
            print_status "Found LXQt version: $lxqt_version"
        fi
    fi
}

# Build the project
build_project() {
    print_status "Building LXQt Weather Widget..."

    # Store current directory
    local original_dir=$(pwd)

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

    # Return to original directory
    cd "$original_dir"

    print_status "Build completed successfully"
}

# Install the plugin
install_plugin() {
    print_status "Installing LXQt Weather Widget..."

    if [ ! -d "build" ]; then
        print_error "Build directory not found. Please run build first."
        exit 1
    fi

    # Store current directory
    local original_dir=$(pwd)
    cd build

    # Install
    print_status "Installing plugin (may require sudo password)..."
    sudo make install

    # Return to original directory
    cd "$original_dir"

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
        "help"|"-h"|"--help")
            # Show help by falling through to default case
            main "invalid"
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
            echo "LXQt Weather Widget Installation Script"
            echo "========================================"
            echo
            echo "Usage: $0 [command]"
            echo
            echo "Commands:"
            echo "  check     - Check and install system dependencies"
            echo "  build     - Build the project (includes dependency check)"
            echo "  install   - Install the built plugin to system"
            echo "  restart   - Restart LXQt panel to load the widget"
            echo "  all       - Complete installation (default - recommended)"
            echo "  help      - Show this help message"
            echo
            echo "Examples:"
            echo "  $0              # Complete automatic installation"
            echo "  $0 all          # Same as above"
            echo "  $0 check        # Only check/install dependencies"
            echo "  $0 build        # Build after dependencies are satisfied"
            echo
            echo "The script will automatically:"
            echo "  • Detect your package manager (apt/pacman/dnf)"
            echo "  • Install required Qt5 and LXQt development packages"
            echo "  • Build and install the weather widget"
            echo "  • Restart LXQt panel to load the new widget"
            echo
            echo "After installation, add the widget through:"
            echo "  Panel → Configure Panel → Widgets → Add Weather Widget"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
