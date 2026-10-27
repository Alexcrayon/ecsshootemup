# Zero Arc

A vertical bullet-hell shoot-'em-up built in C++17 and SDL3 on a custom
entity-component-system (ECS) engine, with enemy behavior scripted in Lua.

Clear three waves of enemies, then beat a three-phase boss.

<!-- Add a screenshot or GIF here, e.g. ![Gameplay](docs/gameplay.gif) -->

## Features

- **Data-driven enemies.** Wave schedules and enemy types are defined in
  `assets/GameData.json`. One Lua script (`assets/scripts/enemy.lua`) produces
  ring, fan, spiral, and aimed bullet patterns from the same burst-angle math,
  so enemies can be tuned without recompiling.
- **Multi-phase boss.** The boss switches bullet patterns and movement as its
  health drops (`assets/scripts/boss.lua`).
- **Wave spawner and game state machine in C++**, covered by Catch2 unit and
  property tests.
- **Uniform-grid collision** for broad-phase checks between the player,
  enemies, and bullets.
- **Audio** through SDL3_mixer: hit sound effects and looping background music.

## Controls

| Key | Action |
| --- | --- |
| Arrow keys / WASD | Move |
| Space | Fire / start / play again |
| Esc | Quit |

## Building

The project is built and tested on Windows with Visual Studio.

### Requirements

- Visual Studio 2022 with the **Desktop development with C++** workload
- CMake 3.20 or newer
- Python 3.10 or newer (for `run.py`)
- Git (CMake downloads Lua, SDL3_mixer, Catch2, and nlohmann/json at configure time)
- [vcpkg](https://github.com/microsoft/vcpkg), with SDL3, SDL3_image, and SDL3_ttf installed:

  ```bash
  vcpkg install sdl3 sdl3-image sdl3-ttf --triplet x64-windows
  ```

### Point the build at vcpkg

`run.py` looks for the vcpkg toolchain file in this order and uses the first
one it finds:

1. The `CMAKE_TOOLCHAIN_FILE` environment variable
2. The `VCPKG_ROOT` environment variable (`<root>/scripts/buildsystems/vcpkg.cmake`)
3. `vcpkg` on your `PATH`
4. A file named `run.local.toolchain` in the repo root that holds the full path
   to `vcpkg.cmake` on one line (this file is git-ignored)

Setting `VCPKG_ROOT` is usually enough.

### Build and run with run.py

```bash
python run.py
```

This opens a menu:

| Option | What it does |
| --- | --- |
| 1. Compile Game | Configures and builds the game |
| 2. Run Game | Builds, then launches the game |
| 3–9 | Builds, then runs the engine and game test suites |
| 10. Package Game | Builds Release and writes a playable zip to `dist/ZeroArc.zip` |
| 11. Toggle Build Type | Switches options 1–9 between Debug (default) and Release |

The executable is written to `CPP/build/game/<Debug or Release>/game.exe`.

### Package a playable build

Choose **Package Game** in `run.py`. It builds Release and puts the
executable, its DLLs, and the `assets/` folder in `dist/ZeroArc.zip`.
Unzip it anywhere and run `ZeroArc.exe`. The game loads assets from the
`assets/` folder next to the executable, and falls back to the repo's
`assets/` folder when run from a development build.

Players need the [Microsoft Visual C++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe),
which most Windows PCs already have.

### Build manually with CMake

From the `CPP` folder:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
```

```bash
cmake --build build
```

Run the tests:

```bash
ctest --test-dir build -C Debug --output-on-failure
```

## Project layout

```
assets/        GameData.json, Lua scripts, images, audio, fonts
CPP/engine/    ECS engine: components, systems, Lua bindings, resource and audio managers
CPP/game/      Game-specific systems (waves, game state, collision response) and main.cpp
run.py         Build, run, and test menu
```

## Credits

- Starter engine code: CS 5850 *Making Game Engines* course material, MIT
  license (see `LICENSE`)
- Pixel art: [Space Ship Shooter Pixel Art Assets](https://opengameart.org/content/space-ship-shooter-pixel-art-assets), OpenGameArt
- Sound effects: [hosch](https://hosch.itch.io)
- Music: "Silver Bullet" by Vitalezzz
- Libraries: SDL3, SDL3_image, SDL3_ttf, SDL3_mixer, Lua, Catch2, nlohmann/json
