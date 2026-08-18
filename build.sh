#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$ROOT"

echo "==> Building Bootstrapper..."
if command -v msbuild >/dev/null 2>&1; then
    msbuild "$ROOT/Bootstrapper/Bootstrapper.sln" \
        /p:Configuration=Release \
        /p:Platform="Any CPU"
elif command -v dotnet >/dev/null 2>&1; then
    dotnet msbuild "$ROOT/Bootstrapper/Bootstrapper.sln" \
        /p:Configuration=Release \
        /p:Platform="Any CPU"
else
    echo "ERROR: msbuild or dotnet was not found."
    exit 1
fi

echo "==> Building manager..."
make -C "$ROOT/manager" clean
make -C "$ROOT/manager"

echo "==> Preparing ModManager.zip..."

rm -f "$ROOT/ModManager.zip"

TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

MOD_DIR="$TEMP_DIR/ModManager"
mkdir -p "$MOD_DIR"

# Bootstrapper
BOOTSTRAPPER="$ROOT/Bootstrapper/Bootstrapper/bin/Release/Bootstrapper.dll"

if [ ! -f "$BOOTSTRAPPER" ]; then
    echo "ERROR: Bootstrapper.dll was not found:"
    echo "       $BOOTSTRAPPER"
    exit 1
fi

cp "$BOOTSTRAPPER" "$MOD_DIR/Bootstrapper.dll"

# Manager executable
if [ ! -f "$ROOT/manager/manager.exe" ]; then
    echo "ERROR: manager/manager.exe was not found."
    exit 1
fi

cp "$ROOT/manager/manager.exe" "$MOD_DIR/manager.exe"

# Resources
if [ -d "$ROOT/manager/res" ]; then
    cp -r "$ROOT/manager/res" "$MOD_DIR/res"
fi

echo "==> Creating ModManager.zip..."
(
    cd "$TEMP_DIR"
    zip -qr "$ROOT/ModManager.zip" ModManager
)

echo "==> Creating ModMgr.zip..."

rm -f "$ROOT/ModMgr.zip"

PACKAGE_DIR="$TEMP_DIR/package"
mkdir -p "$PACKAGE_DIR"

cp "$ROOT/ModManager.zip" "$PACKAGE_DIR/ModManager.zip"
cp "$ROOT/setup.py" "$PACKAGE_DIR/setup.py"

(
    cd "$PACKAGE_DIR"
    zip -qr "$ROOT/ModMgr.zip" ModManager.zip setup.py
)

echo
echo "Build complete:"
echo "  $ROOT/ModManager.zip"
echo "  $ROOT/ModMgr.zip"