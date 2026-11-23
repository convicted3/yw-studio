#!/bin/sh
# Installs YW Studio for the current user (no root): the program in ~/.local/opt/YW, a launcher in ~/.local/bin,
# the menu entry, the icon and the .yw file type. Run it again to update; ./install.sh --uninstall removes it.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
data="${XDG_DATA_HOME:-$HOME/.local/share}"
opt="$HOME/.local/opt/YW"
bin="$HOME/.local/bin"

if [ "$1" = "--uninstall" ]; then
    rm -rf "$opt" "$bin/YW" "$data/applications/yw.desktop" "$data/mime/packages/yw-mime.xml"
    for s in 16 24 32 48 64 128 256 512; do rm -f "$data/icons/hicolor/${s}x${s}/apps/yw.png"; done
    command -v update-mime-database > /dev/null && update-mime-database "$data/mime" || true
    command -v update-desktop-database > /dev/null && update-desktop-database "$data/applications" || true
    echo "YW Studio removed (your settings stay in ${XDG_CONFIG_HOME:-$HOME/.config}/YW)."
    exit 0
fi

mkdir -p "$opt" "$bin" "$data/applications" "$data/icons/hicolor/256x256/apps" "$data/mime/packages"
cp -r "$here"/. "$opt"/
ln -sf "$opt/YW" "$bin/YW"
sed "s|^Exec=.*|Exec=\"$opt/YW\" %F|" "$here/yw.desktop" > "$data/applications/yw.desktop"
if [ -d "$here/icons/hicolor" ]; then
    for s in 16 24 32 48 64 128 256 512; do
        [ -f "$here/icons/hicolor/${s}x${s}/apps/yw.png" ] || continue
        mkdir -p "$data/icons/hicolor/${s}x${s}/apps"
        cp "$here/icons/hicolor/${s}x${s}/apps/yw.png" "$data/icons/hicolor/${s}x${s}/apps/yw.png"
    done
else
    cp "$here/yw.png" "$data/icons/hicolor/256x256/apps/yw.png"
fi
cp "$here/yw-mime.xml" "$data/mime/packages/yw-mime.xml"
command -v update-mime-database > /dev/null && update-mime-database "$data/mime" || true
command -v update-desktop-database > /dev/null && update-desktop-database "$data/applications" || true
command -v gtk-update-icon-cache > /dev/null && gtk-update-icon-cache -q "$data/icons/hicolor" 2> /dev/null || true
echo "YW Studio installed: start it from the applications menu or run $bin/YW"
