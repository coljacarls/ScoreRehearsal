#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# ScoreRehearsal build script (Linux, GCC/Clang + Ninja)
#
# usage: build.sh [configure|build|install|run|clean] [Release|Debug|RelWithDebInfo] [score.mscz]
#
#   configure  run CMake only
#   build      configure + compile
#   install    configure + compile + install into build/<type>/install (default)
#   run        install + start the app (optionally with a score file)
#   clean      remove the build directory
#
# Environment: QT_DIR (default $HOME/Qt/6.11.2/gcc_64)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ACTION="${1:-install}"
BUILD_TYPE="${2:-Release}"
QT_DIR="${QT_DIR:-$HOME/Qt/6.11.2/gcc_64}"

# "release" drops the "Development" suffix from the app name and data directory
APP_BUILD_MODE=dev
if [ "$BUILD_TYPE" = "Release" ]; then APP_BUILD_MODE=release; fi

BUILD_DIR="$ROOT/build/$BUILD_TYPE"
INSTALL_DIR="$BUILD_DIR/install"

if [ "$ACTION" = "clean" ]; then
    rm -rf "$BUILD_DIR"
    exit 0
fi

export PATH="$QT_DIR/bin:$PATH"

cmake -S "$ROOT/musescore" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DMUSESCORE_BUILD_CONFIGURATION=app-external \
    -DMUSE_APP_BUILD_MODE="$APP_BUILD_MODE" \
    -DMUE_EXTERNAL_APP_DIR="$ROOT/src" \
    -DMUE_RUN_WINDEPLOYQT=OFF \
    -DMUSE_COMPILE_USE_UNITY=ON
[ "$ACTION" = "configure" ] && exit 0

ninja -C "$BUILD_DIR"
[ "$ACTION" = "build" ] && exit 0

ninja -C "$BUILD_DIR" install
[ "$ACTION" = "install" ] && exit 0

if [ "$ACTION" = "run" ]; then
    exec "$INSTALL_DIR/bin/ScoreRehearsal" ${3:+"$3"}
fi
