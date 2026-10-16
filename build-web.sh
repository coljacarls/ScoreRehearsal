#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# ScoreRehearsal build script for the web version (Qt for WebAssembly, PWA)
#
# usage: ./build-web.sh [build|package|serve|all]    (default: all)
#
#   build     compile the app and the audio engine (build/web/app, build/web/audio)
#   package   put the web app together in dist/web (also needs a finished build)
#   serve     serve dist/web on http://localhost:8080/
#   all       build + package
#
# Environment:
#   QT_WASM_DIR      Qt for WebAssembly (single thread), default $HOME/Qt/6.11.2/wasm_singlethread
#   QT_HOST_PATH     Qt for the build machine (same version), needed to cross compile
#   EMSDK            the Emscripten SDK that Qt requires (4.0.7 for Qt 6.11), already activated:
#                    source "$EMSDK/emsdk_env.sh"
#   BUILD_TYPE       Release (default) or Debug
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ACTION="${1:-all}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
QT_WASM_DIR="${QT_WASM_DIR:-$HOME/Qt/6.11.2/wasm_singlethread}"
BUILD_DIR="$ROOT/build/web"
DIST_DIR="$ROOT/dist/web"

die() { echo "build-web.sh: $*" >&2; exit 1; }

build() {
    command -v emcc >/dev/null || die "emcc not found: activate the Emscripten SDK first (source \$EMSDK/emsdk_env.sh)"
    [ -f "$QT_WASM_DIR/lib/cmake/Qt6/qt.toolchain.cmake" ] || die "Qt for WebAssembly not found in QT_WASM_DIR=$QT_WASM_DIR"
    [ -f "$ROOT/musescore/CMakeLists.txt" ] || die "the musescore submodule is missing: git submodule update --init --recursive"

    local host_args=()
    [ -n "${QT_HOST_PATH:-}" ] && host_args+=("-DQT_HOST_PATH=$QT_HOST_PATH")

    # Both builds use the toolchain file of Qt (which uses the one of Emscripten)
    export CMAKE_TOOLCHAIN_FILE="$QT_WASM_DIR/lib/cmake/Qt6/qt.toolchain.cmake"

    echo "=== App ==="
    cmake -S "$ROOT/musescore" -B "$BUILD_DIR/app" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_PREFIX_PATH="$QT_WASM_DIR" \
        "${host_args[@]}" \
        -DMUSESCORE_BUILD_CONFIGURATION=app-external \
        -DMUSE_APP_BUILD_MODE="$([ "$BUILD_TYPE" = Release ] && echo release || echo dev)" \
        -DMUE_EXTERNAL_APP_DIR="$ROOT/src"
    cmake --build "$BUILD_DIR/app"

    echo "=== Audio engine ==="
    cmake -S "$ROOT/musescore/src/web/audioengine" -B "$BUILD_DIR/audio" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_PREFIX_PATH="$QT_WASM_DIR" \
        "${host_args[@]}"
    cmake --build "$BUILD_DIR/audio"
}

package() {
    command -v node >/dev/null || die "node not found"

    local app_js audio_js
    app_js="$(find "$BUILD_DIR/app" \( -name ScoreRehearsal.js -o -name MuseScoreStudio.js \) -not -path '*/CMakeFiles/*' | head -n 1)"
    audio_js="$(find "$BUILD_DIR/audio" -name MuseAudio.js -not -path '*/CMakeFiles/*' | head -n 1)"
    [ -n "$app_js" ] || die "no app build found in $BUILD_DIR/app (run: ./build-web.sh build)"
    [ -n "$audio_js" ] || die "no audio engine build found in $BUILD_DIR/audio (run: ./build-web.sh build)"

    node "$ROOT/src/web/tools/package.mjs" \
        --build "$(dirname "$app_js")" \
        --audio "$(dirname "$audio_js")" \
        --out "$DIST_DIR"
}

serve() {
    [ -d "$DIST_DIR" ] || die "$DIST_DIR does not exist (run: ./build-web.sh package)"
    node "$ROOT/src/web/tools/serve.mjs" "$DIST_DIR" 8080
}

case "$ACTION" in
    build) build ;;
    package) package ;;
    serve) serve ;;
    all) build; package ;;
    *) die "unknown action '$ACTION' (build|package|serve|all)" ;;
esac
