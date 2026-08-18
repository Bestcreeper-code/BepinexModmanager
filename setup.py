import re
import shutil
import zipfile
from pathlib import Path


curr_dir = Path.cwd()
zip_path = curr_dir / "ModManager.zip"
mod_manager_dir = curr_dir / "ModManager"


# Find BepInEx

def find_bepinex():
    bepinex_dir = curr_dir / "BepInEx"

    if bepinex_dir.is_dir():
        return bepinex_dir

    raise FileNotFoundError(
        "Could not find BepInEx directory.\n"
        f"Expected BepInEx in the game directory: {curr_dir}"
    )


# Unzip ModManager

if not zip_path.exists():
    raise FileNotFoundError(
        f"ModManager.zip not found: {zip_path}"
    )

with zipfile.ZipFile(zip_path, "r") as z:
    z.extractall(curr_dir)


if not mod_manager_dir.is_dir():
    raise FileNotFoundError(
        f"ModManager directory was not created: {mod_manager_dir}"
    )


# Copy Mono.Cecil.dll from BepInEx

bepinex_dir = find_bepinex()
cecil_source = bepinex_dir / "core" / "Mono.Cecil.dll"
cecil_target = mod_manager_dir / "Mono.Cecil.dll"

if not cecil_source.exists():
    raise FileNotFoundError(
        f"Mono.Cecil.dll not found in BepInEx:\n"
        f"  {cecil_source}"
    )

shutil.copy2(cecil_source, cecil_target)

print(f"Copied: {cecil_source}")
print(f"     -> {cecil_target}")


# Update doorstop_config.ini

ini_path = curr_dir / "doorstop_config.ini"

if not ini_path.exists():
    raise FileNotFoundError(
        f"doorstop_config.ini not found in game directory: {curr_dir}"
    )

text = ini_path.read_text(encoding="utf-8")

text, count = re.subn(
    r"(?m)^(\s*target_assembly\s*=\s*).*$",
    r"\1ModManager/Bootstrapper.dll",
    text,
)

if count == 0:
    raise RuntimeError(
        "target_assembly entry not found in doorstop_config.ini"
    )

ini_path.write_text(text, encoding="utf-8")

print(f"Updated: {ini_path}")