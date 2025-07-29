# LXQt Weather Widget

**Fully AI-generated project with Cursor**

A weather widget for LXQt panel with visualization of weather conditions and temperature based on geolocation.

## ✨ Key Features

- **Automatic location detection** via IP address
- **Temperature display** in Celsius or Fahrenheit
- **Weather icons** for various conditions (clear, cloudy, rain, snow, fog, storm)
- **Weather description** (optional)
- **Configurable update interval** (5-120 minutes)
- **Tooltip with detailed information** (humidity, pressure, wind)
- **Click to refresh** data
- **Free API** - no registration or API keys required

## 🌤️ Data Source

The widget uses **[Open-Meteo.com](https://open-meteo.com/)** - a free weather API:

- ✅ **No registration** - no API keys needed
- ✅ **High accuracy** - data from meteorological services
- ✅ **Free usage** - no request limits
- ✅ **Environmentally friendly** - runs on 100% renewable energy

Geolocation is determined via **ip-api.com** with fallback to Moscow.

## 📋 Requirements

- **GCC 12+** (as specified by user)
- **CMake 3.16+**
- **Qt5** (Core, Widgets, Network)
- **LXQt development libraries** (for full panel integration)

## 🔧 Build and Installation

### Automatic Installation (Recommended)

The installation script will automatically detect and install all required dependencies:

```bash
# Clone repository
git clone <repository-url>
cd lxqt-weather

# Run installation script (handles dependencies automatically)
chmod +x install.sh
./install.sh
```

The script will:
- ✅ Check for required build tools (gcc, g++, cmake, make, pkg-config)
- ✅ Detect and install Qt5 development packages
- ✅ Install LXQt development libraries
- ✅ Build the project
- ✅ Install the widget to system directories
- ✅ Restart LXQt panel automatically

**Supported package managers:** APT (Debian/Ubuntu), Pacman (Arch), DNF (Fedora/RHEL)

### Manual Build

If you prefer manual control:

```bash
# Install dependencies first (see below)
# Then build:
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Manual Dependency Installation

Only needed if not using the automatic installation script:

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qtbase5-dev \
                 libqt5x11extras5-dev liblxqt1-dev
```

#### Fedora/CentOS/RHEL
```bash
sudo dnf install gcc-c++ cmake pkgconfig qt5-qtbase-devel \
                 qt5-qtx11extras-devel lxqt-build-tools lxqt-panel-devel
```

#### Arch Linux
```bash
sudo pacman -S base-devel cmake pkgconf qt5-base qt5-x11extras lxqt-build-tools lxqt-panel
```

## 🎮 Testing

After building, you can test the widget without installing it to the panel:

```bash
# From build directory
./weather-test

# Or after installation
weather-test
```

The test application allows you to:
- View the weather widget in a separate window
- Configure parameters (update interval, temperature units, show description)
- Manually refresh data with the "Refresh" button
- Debug network connectivity issues

### Testing Network Connectivity

If you experience connection issues, you can test the APIs manually:

```bash
# Test geolocation service
curl "http://ip-api.com/json/"

# Test weather API (replace coordinates with your location)
curl "https://api.open-meteo.com/v1/forecast?latitude=55.7558&longitude=37.6176&current_weather=true"
```

## ⚙️ Configuration

The widget doesn't require API key configuration. Main settings are available through the LXQt panel interface:

### Available Parameters:
- **Update Interval**: 5-120 minutes (default: 30 minutes)
- **Temperature Unit**: Celsius/Fahrenheit (default: Celsius)
- **Show Description**: Yes/No (default: Yes)

### Fallback Location:
If automatic location detection is impossible, Moscow (55.7558°N, 37.6176°E) is used.

## 🎯 Usage

1. **Adding to panel**: Right-click on LXQt panel → "Add Widgets" → "Weather"
2. **Refresh data**: Left-click on widget
3. **Settings**: Right-click on widget → "Configure"
4. **Detailed information**: Hover over widget for tooltip

## 🌟 Weather Icons

| Weather Conditions | Icon | Description |
|-------------------|------|-------------|
| Clear | ☀ | Clear sky |
| Partly Cloudy | ⛅ | Variable cloudiness |
| Cloudy | ☁ | Overcast |
| Rain | 🌧 | Precipitation |
| Shower | 🌦 | Brief rain |
| Storm | ⛈ | Thunderstorm activity |
| Snow | 🌨 | Snowfall |
| Fog | 🌫 | Fog/haze |

## 🚨 Troubleshooting

### Widget doesn't display data
1. Check internet connection
2. Ensure ports 80/443 are not blocked
3. Check logs: `journalctl -f | grep weather`

### Compilation error
1. Ensure all dependencies are installed
2. Check GCC version: `gcc --version` (requires 12+)
3. Clear build cache: `rm -rf build && mkdir build`

### Widget doesn't appear in panel
1. Restart LXQt panel: `killall lxqt-panel && lxqt-panel &`
2. Check plugin file permissions
3. Verify correct installation: `ls -la /usr/lib/x86_64-linux-gnu/lxqt-panel/libweather.so`

## 🏗️ Architecture

```
├── src/
│   ├── lxqtweatherplugin.{h,cpp}    # Main LXQt plugin
│   ├── lxqtweatherwidget.{h,cpp}    # UI widget
│   ├── weatherapi.{h,cpp}           # Open-Meteo API client
│   ├── geolocation.{h,cpp}          # Geolocation service
│   ├── main.cpp                     # Test application
│   └── resources.qrc                # Qt resources
├── data/
│   ├── weather.desktop              # Plugin description
│   └── icons/                       # Weather SVG icons
├── CMakeLists.txt                   # Build configuration
└── README.md                        # Documentation
```

### Components:
- **LXQtWeatherPlugin**: Interface with LXQt panel
- **LXQtWeatherWidget**: Main UI widget
- **WeatherAPI**: HTTP client for Open-Meteo API
- **GeoLocation**: IP-based location detection

## 📝 License

MIT License - see [LICENSE](LICENSE) file

## 👥 Authors

- **Developer**: AI Assistant
- **Client**: LXQt panel widget development expert
- **Year**: 2025

## 🔄 Changelog

### v1.1.0 (2025-01-15)
- ✅ **Migration to Open-Meteo API** - removed need for API keys
- ✅ **Simplified setup** - works out of the box
- ✅ **Improved reliability** - free and stable API
- ✅ **Environmental friendliness** - API runs on renewable energy

### v1.0.0 (2025-01-15)
- ✅ Initial implementation with OpenWeatherMap API
- ✅ Basic weather display functionality
- ✅ LXQt panel integration
- ✅ Settings and geolocation support

---

**Note**: Weather data provided by [Open-Meteo.com](https://open-meteo.com/). Geolocation determined via [ip-api.com](http://ip-api.com/). 
