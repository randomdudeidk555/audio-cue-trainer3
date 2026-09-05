cmake_minimum_required(VERSION 3.21)

# Project name must match the mod ID from mod.json (without developer prefix)
project(randomdudeidk555.audio_cue_trainer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Geode SDK
# ---------------------------------------------------------------------------
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "Unable to find Geode SDK! Please define the GEODE_SDK environment variable."
    )
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ---------------------------------------------------------------------------
# Zdroje modu
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Registrace Geode Modu
setup_geode_mod(${PROJECT_NAME}
    SOURCES ${SOURCES}
    DOCS TRUE
)
