# Mod Manager

A Doorstop-based launcher/mod manager for the game. It lets you pick which mods to run
before the game starts, with **BepInEx** as the main/principal mod loader. Additional
loaders can be plugged in as "addons"  right now the only extra addon is
**Gambonanza ModHost** (the game's own modding SDK), on top of BepInEx.

## Installation

1. Build the project (see below) or grab a pre-built `ModManager.zip` + `setup.py`.
2. Make sure **BepInEx** is already installed in your game folder (this manager relies
   on BepInEx being present  it's the principal mod loader).
3. Copy `ModManager.zip` and `setup.py` into your game's root directory (the same folder
   as `doorstop_config.ini`).
4. Run:
   ```
   python setup.py
   ```
   This will:
   - Extract `ModManager.zip` into a `ModManager/` folder.
   - Copy `Mono.Cecil.dll` from `BepInEx/core/` into `ModManager/`.
   - Update `doorstop_config.ini` so `target_assembly` points to
     `ModManager/Bootstrapper.dll`.
5. Launch the game. The manager UI will pop up before the game starts, letting you
   enable/disable mods and choose "Run Modded" or "Run Vanilla".

## Building from source

Requirements:
- `msbuild` or `dotnet` (for the C# Bootstrapper)
- MinGW cross-compiler (`x86_64-w64-mingw32-g++`) and `make` (for the C++ manager)
- `cmake` (to build raylib)
- Git submodules initialized (`git submodule update --init --recursive`)

Then simply run:

```
./build.sh
```

This will:
1. Build the **Bootstrapper** (the Doorstop entrypoint that scans installed BepInEx
   plugins and launches the manager UI).
2. Build the **manager** (the raylib/ImGui GUI app) via `make`.
3. Package everything into `ModManager.zip` (manager + resources + bootstrapper) and
   `ModMgr.zip` (the installable package, including `setup.py`).

Output files, in the repo root:
- `ModManager.zip`  the manager itself, ready to drop into a game folder.
- `ModMgr.zip`  full install package (`ModManager.zip` + `setup.py`) for end users.

### Note on addons

The manager discovers mods through pluggable "searchers":
- **BepInEx**  the principal/default source of mods, scanned via the Bootstrapper.
- **Gambonanza ModHost**  currently the only additional addon, adding support for
  mods managed through the Gambonanza modding SDK (`Mods/*/mod.json`).

Both are built in and enabled by default; no extra setup is needed to use them.

 
> **Note:** only Windows (and Wine/Proton on Linux) is supported right now.