# Game Tests CMakeLists.txt
# This file configures unit and property tests for the game

# Enable testing
enable_testing()

# ============================================================================
# Unit Tests (Catch2)
# ============================================================================

# Glob all unit test files
file(GLOB GAME_UNIT_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp")

# Only create unit test executable if there are test files
if(GAME_UNIT_TESTS)
    # Collect engine source files for unit tests
    file(GLOB ENGINE_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/../../engine/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../engine/ecs/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../engine/ecs/systems/*.cpp"
    )

    # Collect game source files (exclude main.cpp to avoid duplicate main)
    file(GLOB GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../game/*.cpp")
    list(FILTER GAME_SOURCES EXCLUDE REGEX "main\\.cpp$")
    
    add_executable(game_unit_tests 
        ${GAME_UNIT_TESTS}
        ${ENGINE_SOURCES}
        ${GAME_SOURCES}
    )
    
    # Link Catch2, SDL3, SDL3_image, SDL3_ttf, nlohmann_json, and Lua
    target_link_libraries(game_unit_tests PRIVATE
        Catch2::Catch2WithMain
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        SDL3_mixer::SDL3_mixer
        nlohmann_json::nlohmann_json
        lua_static
    )
    
    # Set include directories
    target_include_directories(game_unit_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    )
    
    # Copy the FetchContent-built SDL3_mixer DLL next to the test exe so it can
    # launch (system SDL3/image/ttf are on PATH; the freshly-built mixer is not).
    if(WIN32)
        add_custom_command(TARGET game_unit_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3_mixer::SDL3_mixer>
            $<TARGET_FILE_DIR:game_unit_tests>
            COMMENT "Copying SDL3_mixer DLL next to game_unit_tests"
        )
    endif()

    # Register with CTest
    add_test(NAME Game_Unit_Tests COMMAND game_unit_tests)
endif()

# ============================================================================
# Property Tests (Catch2)
# ============================================================================

# Glob all property test files
file(GLOB GAME_PROPERTY_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/property/*.cpp")

# Only create property test executable if there are test files
if(GAME_PROPERTY_TESTS)
    # Collect engine source files for property tests
    file(GLOB ENGINE_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/../../engine/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../engine/ecs/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../engine/ecs/systems/*.cpp"
    )

    # Collect game source files (exclude main.cpp to avoid duplicate main)
    file(GLOB GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../game/*.cpp")
    list(FILTER GAME_SOURCES EXCLUDE REGEX "main\\.cpp$")
    
    add_executable(game_property_tests 
        ${GAME_PROPERTY_TESTS}
        ${ENGINE_SOURCES}
        ${GAME_SOURCES}
    )
    
    # Link Catch2, SDL3, SDL3_image, SDL3_ttf, nlohmann_json, and Lua
    target_link_libraries(game_property_tests PRIVATE
        Catch2::Catch2WithMain
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        SDL3_mixer::SDL3_mixer
        nlohmann_json::nlohmann_json
        lua_static
    )
    
    # Set include directories
    target_include_directories(game_property_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    )
    
    # Copy the FetchContent-built SDL3_mixer DLL next to the test exe so it can
    # launch (system SDL3/image/ttf are on PATH; the freshly-built mixer is not).
    if(WIN32)
        add_custom_command(TARGET game_property_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3_mixer::SDL3_mixer>
            $<TARGET_FILE_DIR:game_property_tests>
            COMMENT "Copying SDL3_mixer DLL next to game_property_tests"
        )
    endif()

    # Register with CTest
    add_test(NAME Game_Property_Tests COMMAND game_property_tests)
endif()
