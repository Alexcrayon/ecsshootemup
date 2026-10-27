#ifndef PROJECT_PATHS_HPP
#define PROJECT_PATHS_HPP

#include <SDL3/SDL_filesystem.h>

#include <filesystem>
#include <string>

/**
 * Project path resolution.
 *
 * CLASS_ROOT_DIR is injected by CMake as the absolute path to the
 * class root directory (e.g. /Users/you/5850/2026/Class-030).
 * This means asset paths resolve correctly no matter where the
 * executable is launched from.
 *
 * A packaged build has an `assets` folder next to the executable instead,
 * so assets_dir() checks there first and falls back to CLASS_ROOT_DIR.
 *
 * Usage:
 *   #include "engine/project_paths.hpp"
 *   std::string assets = project_paths::assets_dir();  // -> "/abs/path/to/Class-030/assets"
 */
namespace project_paths {

/// Absolute path to the class root directory (where CMakeLists.txt lives)
inline std::string class_root() {
    return CLASS_ROOT_DIR;
}

/// Absolute path to the shared assets directory: <exe dir>/assets if it
/// exists (packaged build), otherwise CLASS_ROOT_DIR/assets (dev build).
inline std::string assets_dir() {
    static const std::string dir = [] {
        if (const char* base = SDL_GetBasePath()) {   // owned by SDL, ends in a separator
            std::string packaged = std::string(base) + "assets";
            std::error_code ec;
            if (std::filesystem::is_directory(packaged, ec)) {
                return packaged;
            }
        }
        return std::string(CLASS_ROOT_DIR) + "/assets";
    }();
    return dir;
}

}  // namespace project_paths

#endif // PROJECT_PATHS_HPP
