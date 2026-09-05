cmake_minimum_required(VERSION 3.21)
project(AudioCueTrainer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Oprava pro Android NDK (C++20/23 v libc++)
# ---------------------------------------------------------------------------
if(ANDROID)
    add_compile_options(-fexperimental-library)
endif()

# ---------------------------------------------------------------------------
# Geode SDK
# ---------------------------------------------------------------------------
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "Unable to find Geode SDK! Please define the GEODE_SDK environment "
        "variable to point to your Geode SDK installation."
    )
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ---------------------------------------------------------------------------
# Zdroje modu
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# ---------------------------------------------------------------------------
# Registrace Geode Modu (důležité: předáváme pouze SOURCES)
# ---------------------------------------------------------------------------
setup_geode_mod(${PROJECT_NAME}
    SOURCES ${SOURCES}
)
