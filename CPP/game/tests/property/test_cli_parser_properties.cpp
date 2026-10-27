/**
 * Property-based tests for CLI parser (cli_parser.hpp)
 *
 * These tests verify four correctness properties from the design document:
 *   Property 1: CLI Parser Round-Trip
 *   Property 2: Unrecognized Arguments Produce Parse Error
 *   Property 3: Invalid Key Action Tokens Produce Parse Error
 *   Property 4: Clear-Logs Last-Wins Semantics
 *
 * Feature: 040-05-command-line-debug
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "game/cli_parser.hpp"

#include <algorithm>
#include <string>
#include <vector>

// Configurable test iteration counts (MANDATORY — workspace policy)
constexpr int NUM_OUTER_TESTS = 10;  // Number of different test scenarios
constexpr int NUM_INNER_TESTS = 5;   // Number of iterations per scenario

// ===========================================================================
// Helper: convert string vector to argc/argv for parse_command_line
// ===========================================================================

static CommandLineOptions parse_args(std::vector<std::string> args) {
    std::vector<char*> argv;
    for (auto& s : args) {
        argv.push_back(s.data());
    }
    return parse_command_line(static_cast<int>(argv.size()), argv.data());
}

// ===========================================================================
// Helper: build a random valid CommandLineOptions from generator values
// ===========================================================================

// ---------------------------------------------------------------------------
// Feature: 040-05-command-line-debug, Property 1: CLI Parser Round-Trip
//
// For any valid CommandLineOptions (random key actions with valid key names
// and non-negative frames, random dump/trace frame lists, optional stop frame,
// random booleans for paused/verbose, random positive FPS or 0, random
// clear_logs), parse_command_line(options_to_argv(opts)) produces an
// equivalent struct with parse_error == false.
//
// **Validates: Requirements 11.1, 11.2, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.3, 9.4, 10.1**
// ---------------------------------------------------------------------------
TEST_CASE("CLI Parser Round-Trip",
          "[cli_parser][property]") {
    SECTION("Round-trip: options_to_argv then parse_command_line reproduces equivalent struct") {
        // Generate random seed values for building a CommandLineOptions
        auto num_keys   = GENERATE(take(NUM_OUTER_TESTS, random(0, 4)));
        auto seed       = GENERATE(take(NUM_INNER_TESTS, random(0, 99999)));

        // Build a random valid CommandLineOptions deterministically from seed
        CommandLineOptions original;

        // Use seed to derive deterministic but varied options
        // Key actions: pick random valid keys and frames
        for (int k = 0; k < num_keys; ++k) {
            KeyAction ka;
            ka.frame = static_cast<uint64_t>((seed + k * 37) % 500);
            ka.key = VALID_KEY_NAMES[static_cast<size_t>((seed + k * 13) % VALID_KEY_NAMES.size())];
            original.keys.push_back(ka);
        }

        // Hover actions: 0-3 hovers with non-negative frames and (possibly
        // negative) world coordinates.
        int num_hovers = (seed + 19) % 4;
        for (int h = 0; h < num_hovers; ++h) {
            HoverAction hv;
            hv.frame = static_cast<uint64_t>((seed + h * 29) % 500);
            // Derive signed coords that can be negative (bottom-left origin).
            hv.x = ((seed + h * 17) % 800) - 400;
            hv.y = ((seed + h * 23) % 600) - 300;
            original.hovers.push_back(hv);
        }

        // Click actions: 0-3 clicks with non-negative frames and (possibly
        // negative) coordinates. Duck Pond does not consume clicks; this
        // exercises the parse/round-trip path to match the other classes.
        int num_clicks = (seed + 23) % 4;
        for (int c = 0; c < num_clicks; ++c) {
            MouseClickAction ca;
            ca.frame = static_cast<uint64_t>((seed + c * 31) % 500);
            ca.x = ((seed + c * 19) % 800) - 400;
            ca.y = ((seed + c * 27) % 600) - 300;
            original.clicks.push_back(ca);
        }

        // Dump frames: 0-3 frames
        int num_dumps = (seed + 7) % 4;
        for (int d = 0; d < num_dumps; ++d) {
            original.dump_frames.push_back(static_cast<uint64_t>((seed + d * 41) % 1000));
        }

        // Trace frames: 0-3 frames
        int num_traces = (seed + 11) % 4;
        for (int t = 0; t < num_traces; ++t) {
            original.trace_frames.push_back(static_cast<uint64_t>((seed + t * 53) % 1000));
        }

        // Stop frame: present ~50% of the time
        if (seed % 2 == 0) {
            original.stop_frame = static_cast<uint64_t>(seed % 5000);
        }

        // Boolean flags
        original.paused   = (seed % 3 == 0);
        original.verbose  = (seed % 5 == 0);

        // FPS: 0 (default) or a positive value
        original.fps = (seed % 4 == 0) ? 0 : (1 + (seed % 120));

        // clear_logs
        original.clear_logs = (seed % 2 == 1);

        // seed: present ~25% of the time
        if (seed % 4 == 1) {
            original.seed = static_cast<uint64_t>(seed % 100000);
        }

        // Convert to argv and parse back
        std::vector<std::string> argv = options_to_argv(original);
        auto roundtripped = parse_args(argv);

        // Verify no parse error
        REQUIRE(roundtripped.parse_error == false);

        // Compare all fields
        REQUIRE(roundtripped.keys.size() == original.keys.size());
        for (size_t i = 0; i < original.keys.size(); ++i) {
            REQUIRE(roundtripped.keys[i].frame == original.keys[i].frame);
            REQUIRE(roundtripped.keys[i].key == original.keys[i].key);
        }

        REQUIRE(roundtripped.hovers.size() == original.hovers.size());
        for (size_t i = 0; i < original.hovers.size(); ++i) {
            REQUIRE(roundtripped.hovers[i].frame == original.hovers[i].frame);
            REQUIRE(roundtripped.hovers[i].x == original.hovers[i].x);
            REQUIRE(roundtripped.hovers[i].y == original.hovers[i].y);
        }

        REQUIRE(roundtripped.clicks.size() == original.clicks.size());
        for (size_t i = 0; i < original.clicks.size(); ++i) {
            REQUIRE(roundtripped.clicks[i].frame == original.clicks[i].frame);
            REQUIRE(roundtripped.clicks[i].x == original.clicks[i].x);
            REQUIRE(roundtripped.clicks[i].y == original.clicks[i].y);
        }

        REQUIRE(roundtripped.dump_frames == original.dump_frames);
        REQUIRE(roundtripped.trace_frames == original.trace_frames);
        REQUIRE(roundtripped.stop_frame == original.stop_frame);
        REQUIRE(roundtripped.paused == original.paused);
        REQUIRE(roundtripped.verbose == original.verbose);
        REQUIRE(roundtripped.fps == original.fps);
        REQUIRE(roundtripped.clear_logs == original.clear_logs);
        REQUIRE(roundtripped.seed == original.seed);
    }
}

// ---------------------------------------------------------------------------
// Feature: 040-05-command-line-debug, Property 2: Unrecognized Arguments Produce Parse Error
//
// For any string that does not match a recognized flag and is not a valid
// value token following a recognized flag, parsing an argv containing that
// string sets parse_error to true.
//
// **Validates: Requirements 1.4**
// ---------------------------------------------------------------------------
TEST_CASE("Unrecognized Arguments Produce Parse Error",
          "[cli_parser][property]") {
    SECTION("Random unrecognized --flags produce parse_error") {
        // Generate a random suffix length and character seed
        auto suffix_len = GENERATE(take(NUM_OUTER_TESTS, random(1, 10)));
        auto char_seed  = GENERATE(take(NUM_INNER_TESTS, random(0, 99999)));

        // Build a random flag that is NOT a recognized flag
        // Recognized flags: --keys, --dump, --trace, --stopframe, --seed,
        //                   --paused, --verbose, --fps, --clear-logs, --no-clear-logs, --help
        std::string flag = "--";
        for (int i = 0; i < suffix_len; ++i) {
            // Use lowercase letters to build random suffixes
            char c = 'a' + static_cast<char>((char_seed + i * 7) % 26);
            flag += c;
        }

        // Skip if we accidentally generated a recognized flag
        static const std::vector<std::string> recognized = {
            "--keys", "--dump", "--trace", "--stopframe", "--seed", "--paused",
            "--verbose", "--fps", "--clear-logs", "--no-clear-logs", "--help"
        };
        bool is_recognized = std::find(recognized.begin(), recognized.end(), flag)
                             != recognized.end();
        if (is_recognized) {
            // Still a valid test — just skip this iteration
            SUCCEED("Skipped: accidentally generated a recognized flag");
            return;
        }

        auto opts = parse_args({"game", flag});
        REQUIRE(opts.parse_error == true);
    }
}

// ---------------------------------------------------------------------------
// Feature: 040-05-command-line-debug, Property 3: Invalid Key Action Tokens Produce Parse Error
//
// For any FRAME:KEY token where FRAME is negative, non-integer, or KEY is
// not in VALID_KEY_NAMES, parsing --keys followed by that token sets
// parse_error to true.
//
// **Validates: Requirements 2.4, 2.5**
// ---------------------------------------------------------------------------
TEST_CASE("Invalid Key Action Tokens Produce Parse Error",
          "[cli_parser][property]") {
    SECTION("Invalid key names produce parse_error") {
        // Generate random invalid key names (random lowercase strings not in VALID_KEY_NAMES)
        auto name_len  = GENERATE(take(NUM_OUTER_TESTS, random(1, 6)));
        auto char_seed = GENERATE(take(NUM_INNER_TESTS, random(0, 99999)));

        // Build a random uppercase key name
        std::string key_name;
        for (int i = 0; i < name_len; ++i) {
            char c = 'A' + static_cast<char>((char_seed + i * 11) % 26);
            key_name += c;
        }

        // Skip if we accidentally generated a valid key name
        bool is_valid = std::find(VALID_KEY_NAMES.begin(), VALID_KEY_NAMES.end(), key_name)
                        != VALID_KEY_NAMES.end();
        if (is_valid) {
            SUCCEED("Skipped: accidentally generated a valid key name");
            return;
        }

        // Use a valid frame number with the invalid key name
        std::string token = "10:" + key_name;
        auto opts = parse_args({"game", "--keys", token});
        REQUIRE(opts.parse_error == true);
    }

    SECTION("Non-integer frame numbers produce parse_error") {
        // Generate random alphabetic strings as invalid frame numbers
        auto str_len   = GENERATE(take(NUM_OUTER_TESTS, random(1, 5)));
        auto char_seed = GENERATE(take(NUM_INNER_TESTS, random(0, 99999)));

        std::string frame_str;
        for (int i = 0; i < str_len; ++i) {
            char c = 'a' + static_cast<char>((char_seed + i * 13) % 26);
            frame_str += c;
        }

        // Pair with a valid key name
        std::string token = frame_str + ":RIGHT";
        auto opts = parse_args({"game", "--keys", token});
        REQUIRE(opts.parse_error == true);
    }

    SECTION("Negative frame numbers produce parse_error") {
        auto neg_frame = GENERATE(take(NUM_OUTER_TESTS, random(-1000, -1)));
        auto key_idx   = GENERATE(take(NUM_INNER_TESTS, random(0, static_cast<int>(VALID_KEY_NAMES.size()) - 1)));

        std::string token = std::to_string(neg_frame) + ":" + VALID_KEY_NAMES[static_cast<size_t>(key_idx)];
        auto opts = parse_args({"game", "--keys", token});
        REQUIRE(opts.parse_error == true);
    }
}

// ---------------------------------------------------------------------------
// Feature: 040-05-command-line-debug, Property 4: Clear-Logs Last-Wins Semantics
//
// For any sequence of --clear-logs and --no-clear-logs flags, the resulting
// clear_logs value equals the value implied by the last such flag in the
// sequence.
//
// **Validates: Requirements 9.7**
// ---------------------------------------------------------------------------
TEST_CASE("Clear-Logs Last-Wins Semantics",
          "[cli_parser][property]") {
    SECTION("Last --clear-logs or --no-clear-logs flag wins") {
        // Generate a random sequence length (1..8)
        auto seq_len = GENERATE(take(NUM_OUTER_TESTS, random(1, 8)));
        auto seed    = GENERATE(take(NUM_INNER_TESTS, random(0, 99999)));

        // Build a random sequence of --clear-logs and --no-clear-logs
        std::vector<std::string> args = {"game"};
        bool last_value = true;  // default

        for (int i = 0; i < seq_len; ++i) {
            bool use_clear = ((seed + i * 17) % 2 == 0);
            if (use_clear) {
                args.push_back("--clear-logs");
                last_value = true;
            } else {
                args.push_back("--no-clear-logs");
                last_value = false;
            }
        }

        auto opts = parse_args(args);
        REQUIRE(opts.parse_error == false);
        REQUIRE(opts.clear_logs == last_value);
    }
}
