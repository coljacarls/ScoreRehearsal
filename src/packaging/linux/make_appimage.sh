#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Turns the install directory of build.sh into a ScoreRehearsal AppImage.
# Derived from MuseScore's buildscripts/ci/linux/tools/make_appimage.sh, reduced to what the
# rehearsal app needs (no portable utils, no update tool, no man pages).
#
# usage: make_appimage.sh <install-dir> <output.AppImage> [x86_64|aarch64]
#
# Environment: QT_DIR (Qt root, e.g. ~/Qt/6.11.2/gcc_64; QT_ROOT_DIR is accepted as well)

set -euo pipefail
trap 'code=$?; echo "error: make_appimage.sh: \`$BASH_COMMAND\` exited with code $code" >&2' ERR

INSTALL_DIR="${1:?install dir missing}"
APPIMAGE_NAME="${2:?output name missing}"
PACKARCH="${3:-x86_64}"
QT_DIR="${QT_DIR:-${QT_ROOT_DIR:?QT_DIR is not set}}"

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
BUILD_TOOLS="${BUILD_TOOLS:-$HOME/build_tools}"
INSTALL_DIR="$(cd "$INSTALL_DIR" && pwd)"
OUT_DIR="$(cd "$(dirname "$APPIMAGE_NAME")" && pwd)"
APPIMAGE_NAME="$OUT_DIR/$(basename "$APPIMAGE_NAME")"

# The tools below are AppImages themselves; run them without FUSE (CI runners have none)
export APPIMAGE_EXTRACT_AND_RUN=1

##########################################################################
# TOOLS
##########################################################################

download_tool() { # <repo> <name>
    local repo="$1" name="$2"
    local dir="$BUILD_TOOLS/$name"
    if [ ! -x "$dir/$name" ]; then
        mkdir -p "$dir"
        curl -fL "https://github.com/$repo/releases/download/continuous/$name-$PACKARCH.AppImage" -o "$dir/$name"
        chmod +x "$dir/$name"
    fi
    export PATH="$dir:$PATH"
}

download_tool AppImage/appimagetool appimagetool
download_tool linuxdeploy/linuxdeploy linuxdeploy
download_tool linuxdeploy/linuxdeploy-plugin-qt linuxdeploy-plugin-qt

##########################################################################
# BUNDLE INTO APPDIR (= install dir)
##########################################################################

appdir="$INSTALL_DIR"
cd "$(dirname "$appdir")"

# linuxdeploy expects an AppDir with usr/; the install dir already has bin/ lib/ share/ at its root
[ -e "$appdir/usr" ] || ln -s . "$appdir/usr"

cp "$HERE/de.carls.ScoreRehearsal.desktop" "$appdir/"
cp "$ROOT/src/branding/scorerehearsal_logo.png" "$appdir/ScoreRehearsal.png"

# Qt plugins for MySQL/PostgreSQL would make linuxdeploy-plugin-qt fail on missing libraries
sql_dir="$QT_DIR/plugins/sqldrivers"
sql_tmp="$(mktemp -d)"
for drv in libqsqlmysql.so libqsqlpsql.so; do
    [ -f "$sql_dir/$drv" ] && mv "$sql_dir/$drv" "$sql_tmp/$drv"
done
restore_sql() { for f in "$sql_tmp"/*; do [ -f "$f" ] && mv "$f" "$sql_dir/"; done; true; }
trap restore_sql EXIT

# libqxcb.so is always deployed; Wayland stays available via MU_QT_QPA_PLATFORM=wayland
export EXTRA_PLATFORM_PLUGINS="libqoffscreen.so;libqwayland.so"
# qmlimportscanner looks for QML imports here (app, fork and framework)
export QML_SOURCES_PATHS="$ROOT/src:$ROOT/musescore/src:$ROOT/musescore/muse"
export LD_LIBRARY_PATH="$appdir/lib:$appdir/usr/lib:$QT_DIR/lib:${LD_LIBRARY_PATH:-}"
export QMAKE="$QT_DIR/bin/qmake"

linuxdeploy --appdir "$appdir" \
    --executable "$appdir/bin/ScoreRehearsal" \
    --desktop-file "$appdir/de.carls.ScoreRehearsal.desktop" \
    --icon-file "$appdir/ScoreRehearsal.png" \
    --icon-filename ScoreRehearsal
linuxdeploy-plugin-qt --appdir "$appdir"

# The system's glib has to be used, a bundled one breaks theme and file dialog integration
for lib in libglib-2.0.so.0 libgio-2.0.so.0 libgmodule-2.0.so.0 libgobject-2.0.so.0; do
    rm -f "$appdir/lib/$lib"
done
# https://github.com/musescore/MuseScore/issues/24068#issuecomment-2297823192
rm -f "$appdir/lib/libwayland-client.so.0"

# Qt plugins that linuxdeploy-plugin-qt does not pick up by itself
for plugin in platformthemes/libqgtk3.so \
              wayland-decoration-client wayland-graphics-integration-client wayland-shell-integration; do
    if [ -e "$QT_DIR/plugins/$plugin" ] && [ ! -e "$appdir/plugins/$plugin" ]; then
        mkdir -p "$appdir/plugins/$(dirname "$plugin")"
        cp -Lr "$QT_DIR/plugins/$plugin" "$appdir/plugins/$plugin"
    fi
done

# Fallback libraries: bundled, but AppRun only uses them if the system has no copy
for lib in libOpenGL.so.0; do # https://bugreports.qt.io/browse/QTBUG-89754
    src="$(ldconfig -p | awk -v l="$lib" '$1 == l { print $NF; exit }')"
    if [ -n "$src" ]; then
        mkdir -p "$appdir/fallback/$lib"
        cp -L "$src" "$appdir/fallback/$lib/$lib"
        rm -f "$appdir/lib/$lib"
    fi
done

# Our entry point replaces the one from linuxdeploy
rm -f "$appdir/AppRun"
cp "$HERE/AppRun" "$appdir/AppRun"
chmod +x "$appdir/AppRun"

##########################################################################
# APPIMAGE
##########################################################################

ARCH="$PACKARCH" appimagetool --no-appstream "$appdir" "$APPIMAGE_NAME"
chmod +x "$APPIMAGE_NAME"
echo "Created $APPIMAGE_NAME"
