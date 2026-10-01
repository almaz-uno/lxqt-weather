cmake_minimum_required(VERSION 3.16.0)
# The version comes from the git tag, see target "version" (specs/001-releases)
project(lxqt-weather LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Message for diagnostics
message(STATUS "Configuring LXQt Weather Plugin")

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network Test)

# Find LXQt
find_package(PkgConfig REQUIRED)
pkg_check_modules(LXQT REQUIRED lxqt)

message(STATUS "Found Qt5: ${Qt5_VERSION}")
message(STATUS "Found LXQt: ${LXQT_VERSION}")

# Version from git: the last tag, commits since it, -dirty; "dev" outside a
# repository. Regenerated on every build, rewritten only when it changes
set(VERSION_DIR ${CMAKE_BINARY_DIR}/generated)
add_custom_target(version
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DOUTPUT=${VERSION_DIR}/version.h
            -P ${CMAKE_SOURCE_DIR}/cmake/version.cmake
    BYPRODUCTS ${VERSION_DIR}/version.h
)

# LXQt panel plugin library
add_library(weather SHARED
    src/weatherapi.cpp
    src/geolocation.cpp
    src/geocoder.cpp
    src/locationsetting.cpp
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
target_include_directories(weather PRIVATE src ${VERSION_DIR})
target_include_directories(weather PRIVATE ${LXQT_INCLUDE_DIRS})
target_compile_options(weather PRIVATE ${LXQT_CFLAGS_OTHER})
add_dependencies(weather version)

# Add test executable
add_executable(weather-test
    src/main.cpp
    src/weatherapi.cpp
    src/geolocation.cpp
    src/geocoder.cpp
    src/locationsetting.cpp
    src/lxqtweatherwidget.cpp
)

target_link_libraries(weather-test
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

target_include_directories(weather-test PRIVATE src ${VERSION_DIR})
add_dependencies(weather-test version)

# Text of a GitHub release from RELEASE-NOTES.adoc; not installed
add_library(relnotes-lib STATIC tools/relnotes/relnotes.cpp)
target_link_libraries(relnotes-lib PUBLIC Qt5::Core)
target_include_directories(relnotes-lib PUBLIC tools/relnotes)

add_executable(relnotes tools/relnotes/main.cpp)
target_link_libraries(relnotes PRIVATE relnotes-lib)

# Tests: Qt Test programs under CTest, without a display or the network
enable_testing()

add_executable(relnotes_test tests/relnotes_test.cpp)
target_link_libraries(relnotes_test PRIVATE relnotes-lib Qt5::Test)
target_compile_definitions(relnotes_test PRIVATE
    RELEASE_NOTES="${CMAKE_SOURCE_DIR}/RELEASE-NOTES.adoc")
add_test(NAME relnotes COMMAND relnotes_test)

# The location setting and the geocoder, on recorded responses
# (specs/002-location-by-address)
add_executable(geocoder_test tests/geocoder_test.cpp src/geocoder.cpp src/locationsetting.cpp)
target_link_libraries(geocoder_test PRIVATE Qt5::Core Qt5::Network Qt5::Test)
target_include_directories(geocoder_test PRIVATE src ${VERSION_DIR})
target_compile_definitions(geocoder_test PRIVATE
    FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests")
add_dependencies(geocoder_test version)
add_test(NAME geocoder COMMAND geocoder_test)

# Installation
install(TARGETS weather LIBRARY DESTINATION /usr/lib/x86_64-linux-gnu/lxqt-panel)
install(TARGETS weather-test RUNTIME DESTINATION bin)

# Install .desktop file for LXQt panel
install(FILES data/weather.desktop DESTINATION /usr/share/lxqt/lxqt-panel)

# Debian package of a release: scripts/release.sh passes the version of the
# tag with cpack -D CPACK_PACKAGE_VERSION=X.Y.Z
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME lxqt-weather)
set(CPACK_PACKAGE_VERSION 0.0.0)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Weather widget for the LXQt panel")
set(CPACK_PACKAGE_CONTACT "Maxim Kovrov <maksim.kovrov@gmail.com>")
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION x11)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE https://github.com/almaz-uno/lxqt-weather)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS lxqt-panel)
include(CPack)

message(STATUS "Weather library configured successfully")
