cmake_minimum_required(VERSION 3.21)

# Název projektu (přebírá se automaticky do ${PROJECT_NAME})
project(audio_cue_trainer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# OPRAVA PRO ANDROID (povolení C++20/23 funkcí v libc++)
# ---------------------------------------------------------------------------
if(ANDROID)
    add_compile_options(-fexperimental-library)
endif()

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

# ---------------------------------------------------------------------------
# Vytvoření knihovny + registrace Geode Modu
# ---------------------------------------------------------------------------
add_library(${PROJECT_NAME} SHARED ${SOURCES})
setup_geode_mod(${PROJECT_NAME})
