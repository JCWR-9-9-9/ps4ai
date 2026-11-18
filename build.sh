#!/bin/bash
# Native PS4 build using OpenOrbis Toolchain
set -euo pipefail

cd "$(dirname "$0")"

export OO_PS4_TOOLCHAIN="${OO_PS4_TOOLCHAIN:-$(pwd)/PS4Toolchain}"
export LD_LIBRARY_PATH="$(pwd)/clang-local/usr/lib/x86_64-linux-gnu:$(pwd)/ssl-local/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1

echo "Building PS4 Snake game package..."
make pkg -j4

echo ""
echo "Build complete. Output: $(pwd)/IV0000-BREW00001_00-SNAKEGAME0000000.pkg"

