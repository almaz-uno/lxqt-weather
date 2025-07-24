cmake_minimum_required(VERSION 3.16.0)
project(lxqt-weather VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Message for diagnostics
message(STATUS "Configuring LXQt Weather Plugin")

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

# Find LXQt
find_package(PkgConfig REQUIRED)
pkg_check_modules(LXQT REQUIRED lxqt)

message(STATUS "Found Qt5: ${Qt5_VERSION}")
message(STATUS "Found LXQt: ${LXQT_VERSION}")

# LXQt panel plugin library
add_library(weather SHARED
    src/weatherapi.cpp
    src/geolocation.cpp
    src/lxqtweatherwidget.cpp
    src/lxqtweatherplugin.cpp
)

# Enable MOC for the plugin
set_target_properties(weather PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Enable Qt5 tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Link libraries
target_link_libraries(weather
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    ${LXQT_LIBRARIES}
)

# Include directories
target_include_directories(weather PRIVATE src)
target_include_directories(weather PRIVATE ${LXQT_INCLUDE_DIRS})
target_compile_options(weather PRIVATE ${LXQT_CFLAGS_OTHER})

# Add test executable
add_executable(weather-test
    src/main.cpp
    src/weatherapi.cpp
    src/geolocation.cpp
    src/lxqtweatherwidget.cpp
)

target_link_libraries(weather-test
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

target_include_directories(weather-test PRIVATE src)

# Installation
install(TARGETS weather LIBRARY DESTINATION /usr/lib/x86_64-linux-gnu/lxqt-panel)
install(TARGETS weather-test RUNTIME DESTINATION bin)

# Install .desktop file for LXQt panel
install(FILES data/weather.desktop DESTINATION /usr/share/lxqt/lxqt-panel)

message(STATUS "Weather library configured successfully")
