/**
 * Unit tests for LuaManager class
 *
 * These tests verify LuaManager lifecycle, script loading, error handling,
 * sandbox enforcement, and string execution.
 *
 * Requirements tested: 2.1, 2.2, 2.3, 2.4, 3.1, 3.2, 3.3, 3.4,
 *                      4.1, 4.2, 4.3, 4.4, 14.1, 14.2, 14.3, 14.4, 14.5
 */

#include <catch2/catch_test_macros.hpp>
#include "engine/lua_manager.hpp"
#include <string>

static std::string test_assets_dir() {
    return std::string(CLASS_ROOT_DIR) + "/CPP/engine/tests/test_assets";
}

TEST_CASE("LuaManager construct and destroy", "[lua_manager]") {
    LuaManager mgr;
    CHECK(mgr.state() != nullptr);
}

TEST_CASE("LuaManager load valid script", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.load_script(test_assets_dir() + "/valid_script.lua");
    CHECK(result.success);
    CHECK(result.error_message.empty());
}

TEST_CASE("LuaManager load non-existent file", "[lua_manager]") {
    LuaManager mgr;
    std::string bad_path = "/nonexistent/path.lua";
    auto result = mgr.load_script(bad_path);
    CHECK_FALSE(result.success);
    CHECK(result.error_message.find(bad_path) != std::string::npos);
}

TEST_CASE("LuaManager syntax error reporting", "[lua_manager]") {
    LuaManager mgr;
    std::string path = test_assets_dir() + "/syntax_error.lua";
    auto result = mgr.load_script(path);
    CHECK_FALSE(result.success);
    CHECK(result.error_message.find("syntax_error.lua") != std::string::npos);
}

TEST_CASE("LuaManager runtime error reporting", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("error('test error')");
    CHECK_FALSE(result.success);
    CHECK(result.error_message.find("test error") != std::string::npos);
}

TEST_CASE("LuaManager sandbox blocks os", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("os.execute('echo hi')");
    CHECK_FALSE(result.success);
}

TEST_CASE("LuaManager sandbox blocks io", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("io.open('test')");
    CHECK_FALSE(result.success);
}

TEST_CASE("LuaManager sandbox blocks loadfile", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("loadfile('test.lua')");
    CHECK_FALSE(result.success);
}

TEST_CASE("LuaManager sandbox blocks dofile", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("dofile('test.lua')");
    CHECK_FALSE(result.success);
}

TEST_CASE("LuaManager sandbox allows math", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("local x = math.sqrt(4)");
    CHECK(result.success);
}

TEST_CASE("LuaManager sandbox allows string", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("local x = string.len('hello')");
    CHECK(result.success);
}

TEST_CASE("LuaManager sandbox allows table", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("local t = {}; table.insert(t, 1)");
    CHECK(result.success);
}

TEST_CASE("LuaManager execute string", "[lua_manager]") {
    LuaManager mgr;
    auto result = mgr.execute_string("local x = 1 + 2");
    CHECK(result.success);
}
