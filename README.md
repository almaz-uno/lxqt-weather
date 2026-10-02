# LXQt Weather Widget

**Fully AI-generated project with Cursor**

A weather widget for LXQt panel with visualization of weather conditions and temperature based on geolocation.

## ✨ Key Features

- **Location** set by an address or coordinates, or detected by IP address
- **Temperature display** in Celsius or Fahrenheit
- **Weather icons** for various conditions (clear, cloudy, rain, snow, fog, storm)
- **Humidity and pressure** under the temperature (optional), pressure in mmHg
- **Configurable update interval** (5-120 minutes)
- **Tooltip with detailed information** (humidity, pressure, wind in m/s, conditions)
- **Click to refresh** data
- **Free API** - no registration or API keys required

## 🌤️ Data Source

The widget uses **[Open-Meteo.com](https://open-meteo.com/)** - a free weather API:

- ✅ **No registration** - no API keys needed
- ✅ **High accuracy** - data from meteorological services
- ✅ **Free usage** - no request limits
- ✅ **Environmentally friendly** - runs on 100% renewable energy

A location set in the settings is found once via **Nominatim** (OpenStreetMap); without one, geolocation is determined via **ip-api.com** with fallback to Moscow.

## 📋 Requirements

- **GCC 12+** (as specified by user)
- **CMake 3.16+**
- **Qt5** (Core, Widgets, Network, Test)
- **LXQt development libraries** (for full panel integration)

## 📦 Install from a Release

Every version is a [GitHub release](https://github.com/almaz-uno/lxqt-weather/releases)
with a Debian package built on Debian 12 for amd64 (LXQt 1.2, Qt 5.15) and its checksum:

```bash
# In the directory with the downloaded lxqt-weather_X.Y.Z_amd64.deb and SHA256SUMS
sha256sum -c SHA256SUMS
sudo apt install ./lxqt-weather_X.Y.Z_amd64.deb
killall lxqt-panel && lxqt-panel &
```

The package installs the same files as `install.sh` and takes over a copy
installed by it; the panel keeps the widget and its settings. Remove it with
`sudo apt remove lxqt-weather`. The installed version is shown at the bottom
of the widget's settings dialog and printed by `weather-test --version`.

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
ctest --output-on-failure
sudo make install
```

The version of a build is `git describe --tags --always --dirty` of the
working copy, `dev` outside a repository.

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

# The version of the build, without a display
weather-test --version

# With the "Location" field filled and applied at start
weather-test --location "Технопарк Сколково"
```

The test application allows you to:
- View the weather widget in a separate window
- Configure parameters (update interval, temperature units, humidity and pressure, location)
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
- **Show Humidity and Pressure**: Yes/No (default: Yes) — the line under the temperature, `61% · 756 mmHg`: the relative humidity and the pressure at the surface in millimetres of mercury. The weather conditions are shown by the glyph and in the tooltip
- **Location**: an address, coordinates, or empty (default: empty)

### Location:
- **Empty** — the location is found by your IP address (ip-api.com). Behind a
  VPN that is the VPN's exit.
- **An address** — searched once, when you press OK, in
  [Nominatim](https://nominatim.openstreetmap.org/) (OpenStreetMap). The place
  found is shown under the field, and its coordinates are stored; nothing is sent
  to ip-api.com afterwards. If the address is not found, the dialog stays open
  with the reason. Check the place shown: for an address OpenStreetMap does not
  know, Nominatim may find only the street. A name often works better than a
  house number — `Технопарк Сколково` rather than `Большой бульвар 42с1`.
- **Coordinates** — latitude and longitude in decimal degrees, separated by a
  comma or spaces: `55.6928, 37.3475`. Used as they are, with no search.

The settings are kept in the panel configuration, `~/.config/lxqt/panel.conf`,
in the plugin's section: `location`, `location_latitude`, `location_longitude`,
`location_name`. The key `geocoder_url` (default
`https://nominatim.openstreetmap.org`) points the search at another
Nominatim-compatible service, without a new build.

Addresses are found by Nominatim, © OpenStreetMap contributors, data under the
[ODbL](https://www.openstreetmap.org/copyright).

### Fallback Location:
If the location is not set and cannot be found by the IP address, Moscow
(55.7558°N, 37.6176°E) is used.

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
│   ├── geocoder.{h,cpp}             # Address search (Nominatim)
│   ├── locationsetting.{h,cpp}      # The "Location" setting
│   ├── weathersettings.h            # Settings interface
│   ├── weatherformat.{h,cpp}        # Humidity and pressure as shown
│   ├── main.cpp                     # Test application
│   └── resources.qrc                # Qt resources
├── data/
│   ├── weather.desktop              # Plugin description
│   └── icons/                       # Weather SVG icons
├── cmake/version.cmake              # Version of a build from git
├── tools/relnotes/                  # Release text from RELEASE-NOTES.adoc
├── tests/                           # Qt Test programs, run by ctest
├── scripts/                         # Release script, CI build dependencies
├── specs/                           # Specifications and the constitution
├── CMakeLists.txt                   # Build configuration
├── RELEASE-NOTES.adoc               # Release notes
└── README.md                        # Documentation
```

### Components:
- **LXQtWeatherPlugin**: Interface with LXQt panel
- **LXQtWeatherWidget**: Main UI widget
- **WeatherAPI**: HTTP client for Open-Meteo API
- **GeoLocation**: IP-based location detection
- **LocationSetting**, **Geocoder**: the location set by an address or coordinates

## 📝 License

MIT License - see [LICENSE](LICENSE) file

## 👥 Authors

- **Developer**: AI Assistant
- **Client**: LXQt panel widget development expert
- **Year**: 2025

## 🔄 Release Notes

What each version changed: [RELEASE-NOTES.adoc](RELEASE-NOTES.adoc).

---

**Note**: Weather data provided by [Open-Meteo.com](https://open-meteo.com/). Addresses found by [Nominatim](https://nominatim.openstreetmap.org/), © OpenStreetMap contributors. Geolocation determined via [ip-api.com](http://ip-api.com/). 
