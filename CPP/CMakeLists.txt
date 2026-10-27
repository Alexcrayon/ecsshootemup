# CMake configuration for Class-070: Lua Scripting & Duck Pond
# Self-contained build -- run: cmake -B build -S . from this directory

cmake_minimum_required(VERSION 3.20...4.0)

project(110FinalProjectBulletHell VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compiler warnings so students get feedback on unhandled enum cases, etc.
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# CLASS_ROOT_DIR points to the class root (one level up from CPP/)
add_compile_definitions(CLASS_ROOT_DIR="${CMAKE_SOURCE_DIR}/..")

add_compile_definitions(_USE_MATH_DEFINES)
# Find SDL3 packages
find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

# Build the FetchContent dependencies (Catch2, SDL3_mixer, ...) as STATIC libs.
# SDL3_mixer's CMake sets BUILD_SHARED_LIBS=ON in the cache; on a reconfigure that
# flips Catch2 to a shared Catch2Maind.dll that is NOT beside the test exes, so
# every Catch2-linked test fails to launch (0xc0000135 / "cannot open Catch2Maind.dll").
# Forcing it OFF here — before Catch2 is made available — keeps the test framework
# statically linked and self-contained (and links SDL3_mixer in, no extra DLL).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Fetch dependencies
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch Lua 5.4 source and build as a static C library
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.7
)
FetchContent_MakeAvailable(lua)

# Build Lua as a static C library from source
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
# Remove lua.c (standalone interpreter) and luac.c (compiler) — we only want the library
list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")

add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC ${lua_SOURCE_DIR})
# Compile as C, not C++
set_target_properties(lua_static PROPERTIES LINKER_LANGUAGE C)

# SDL3_mixer -- audio mixing library (ported from the Class-070 audio option).
# MP3 uses the bundled, header-only drmp3 decoder (no external dependency), so
# metalthunk.mp3 loads without extra system libraries.
set(SDL3MIXER_VORBIS VENDORED CACHE STRING "" FORCE)
set(SDL3MIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDL3MIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDL3MIXER_MP3 ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG        release-3.2.0
    GIT_SHALLOW    TRUE
    SYSTEM
)
FetchContent_MakeAvailable(SDL3_mixer)

# Enable testing
enable_testing()

# ============================================================================
# Engine and Game
# ============================================================================

add_subdirectory(engine/tests)
add_subdirectory(game)
