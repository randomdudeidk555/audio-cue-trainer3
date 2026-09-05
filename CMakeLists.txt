cmake_minimum_required(VERSION 3.21)
project(AudioCueTrainer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Oprava pro Android NDK (zpřístupní std::atomic_ref a C++20/23 prvky)
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
        "variable to point to your Geode SDK installation "
        "(see https://docs.geode-sdk.org for setup instructions)."
    )
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ---------------------------------------------------------------------------
# Zdroje a Nastavení Geode Modu
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# NEPOUŽÍVAT add_library! Geode si vytvoří target automaticky.
setup_geode_mod(${PROJECT_NAME} SOURCES ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
