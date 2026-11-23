#!/usr/bin/env bash
# Cross build of the YW ASIO driver with Zig (clang + MinGW-w64). REA rounds 97 and 106.
#
#   scripts/build-yw-asio.sh   -> $OUT/yw-asio.dll (64-bit hosts) and $OUT/yw-asio32.dll (32-bit hosts)
#
# OUT defaults to build-win. Install for other programs: copy the DLLs somewhere permanent and run, as administrator,
#   %SystemRoot%\System32\regsvr32.exe "C:\path\to\yw-asio.dll"      (64-bit hosts)
#   %SystemRoot%\SysWOW64\regsvr32.exe "C:\path\to\yw-asio32.dll"    (32-bit hosts)
# (regsvr32 /u ... removes them; "Register YW ASIO.bat" does both.)
# On 32-bit x86 the IASIO methods are __thiscall, like MSVC's: clang uses thiscall for member functions on
# *-windows-gnu targets (checked in REA_rodada106.md), and the COM entry points are exported undecorated (.def).
set -euo pipefail
cd "$(dirname "$0")/.."
ZIG="${ZIG:-$HOME/opt/zig-x86_64-linux-0.14.1/zig}"
OUT="${OUT:-build-win}"
mkdir -p "$OUT"
for arch in x86_64 x86; do
    dll="$OUT/yw-asio.dll"
    [[ $arch == x86 ]] && dll="$OUT/yw-asio32.dll"
    "$ZIG" c++ -target $arch-windows-gnu -std=c++20 -O2 -DNDEBUG=1 -fno-sanitize=undefined -Wall -Wextra \
        -DUNICODE -D_UNICODE -Ithird_party/asiosdk/common \
        -shared -o "$dll" asio/YwAsio.cpp asio/yw-asio.def \
        -lole32 -luuid -lavrt -luser32 -lgdi32 -ladvapi32 -static
    echo "built $dll"
done
