# Writes the version of the build into a header (specs/001-releases):
#   cmake -DSOURCE_DIR=<repository> -DOUTPUT=<header> -P version.cmake
# The version is `git describe --tags --always --dirty`, "dev" outside a
# repository. The header is rewritten only when the text changes, so a build
# without changes recompiles nothing.

execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0 OR version STREQUAL "")
    set(version "dev")
endif()

set(text "#define LXQT_WEATHER_VERSION \"${version}\"\n")
set(old "")
if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" old)
endif()
if(NOT old STREQUAL text)
    file(WRITE "${OUTPUT}" "${text}")
endif()
