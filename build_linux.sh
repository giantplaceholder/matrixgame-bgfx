#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BX_URL="https://github.com/bkaradzic/bx/archive/59a1e105b6f5dc4600c8bdc24b32499e135df73c.tar.gz"
BX_SHA256="753db194f0b975cdc7e4824a90de1be9566256e2e091d12b4a6118a55e76c759"
BIMG_URL="https://github.com/bkaradzic/bimg/archive/671fe06dd7012a1ab85aa931019ed16112909756.tar.gz"
BIMG_SHA256="8149e79b567db0ba5d7246910ffa82d13b03b6192ce69fefd3a82e07d13ec7e3"
BGFX_URL="https://github.com/bkaradzic/bgfx/archive/73183bea9b92766cfaac2e6f757d0bcc3c0253ef.tar.gz"
BGFX_SHA256="df8cb756e1552c2608abda18addc62035618dc3a26e835ee692b8519ea1daf06"
BGFX_CMAKE_URL="https://github.com/bkaradzic/bgfx.cmake/archive/dba713af2d5a9acd7bd6d6a87498fd8751db6015.tar.gz"
BGFX_CMAKE_SHA256="8870e62b5048d32cb7dcbcd634bd903d99386404927fd23a7da9386751495f50"

SDL_VERSION="2.30.10"
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.tar.gz"
SDL_SHA256="f59adf36a0fcf4c94198e7d3d776c1b3824211ab7aeebeb31fe19836661196aa"
ZLIB_URL="https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz"
ZLIB_SHA256="629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff"
PNG_URL="https://github.com/glennrp/libpng/archive/refs/tags/v1.6.37.tar.gz"
PNG_SHA256="ca74a0dace179a8422187671aee97dd3892b53e168627145271cad5b5ac81307"

SRC="${MG_SRC:-$SCRIPT_DIR}"
WORK="${MG_WORK:-$SCRIPT_DIR/build/linux}"
DEPS="$WORK/deps"
PIC_PREFIX="$WORK/pic"
CACHE="$WORK/cache"
ARCH="${MG_ARCH:-$(uname -m)}"
OUT="${MG_OUT:-$SCRIPT_DIR/out}/linux/$ARCH"

MODE="${MG_MODE:-both}"
die() { echo "build_linux: $*" >&2; exit 1; }
log() { echo "==> $*"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --mode=*) MODE="${1#*=}" ;;
        --mode) shift; MODE="${1:?--mode needs exe, dll or both}" ;;
        -h|--help) sed -n '2,16p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) die "unknown argument: $1" ;;
    esac
    shift
done
case "$MODE" in exe|dll|both) ;; *) die "--mode must be exe, dll or both (got '$MODE')" ;; esac

need() { command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"; }
need curl
need tar
need sha256sum
need cmake
need pkg-config
need c++

jobs_count() {
    if [[ -n "${MG_JOBS:-}" ]]; then echo "$MG_JOBS"; return; fi
    local cpus mem_kb per_job_gb=2 n
    cpus="$(nproc 2>/dev/null || echo 2)"
    mem_kb="$(awk '/MemAvailable/ {print $2}' /proc/meminfo 2>/dev/null || echo 2097152)"
    n=$(( mem_kb / (per_job_gb * 1024 * 1024) ))
    (( n < 1 )) && n=1
    (( n > cpus )) && n=cpus
    echo "$n"
}

sha_ok() { echo "$1  $2" | sha256sum -c --status; }

# Download one tarball, refuse a bad hash, unpack it with the top directory stripped.
fetch_archive() {
    local name="$1" url="$2" sum="$3" dest="$4"
    local arc="$CACHE/${name}.tar.gz" stamp="$dest/.mg-fetch-sha"
    if [[ -f "$stamp" && "$(cat "$stamp")" == "$sum" && -d "$dest" ]]; then
        return 0
    fi
    mkdir -p "$CACHE"
    if [[ ! -f "$arc" ]] || ! sha_ok "$sum" "$arc"; then
        log "downloading $name"
        curl -fsSL --retry 3 -L -o "$arc.part" "$url"
        mv -f "$arc.part" "$arc"
        sha_ok "$sum" "$arc" || { rm -f "$arc"; die "SHA256 mismatch for $name"; }
    fi
    log "unpacking $name"
    rm -rf "$dest"
    mkdir -p "$dest"
    tar --no-same-owner -xzf "$arc" -C "$dest" --strip-components=1
    echo "$sum" > "$stamp"
}

[[ -f "$SRC/platform/CMakeLists.txt" ]] || die "source tree not found at $SRC (set MG_SRC to the MatrixGame directory)"

mkdir -p "$WORK"
JOBS="$(jobs_count)"
log "source $SRC  mode=$MODE  jobs=$JOBS"

fetch_archive bx         "$BX_URL"         "$BX_SHA256"         "$DEPS/bx"
fetch_archive bimg       "$BIMG_URL"       "$BIMG_SHA256"       "$DEPS/bimg"
fetch_archive bgfx       "$BGFX_URL"       "$BGFX_SHA256"       "$DEPS/bgfx"
fetch_archive bgfx.cmake "$BGFX_CMAKE_URL" "$BGFX_CMAKE_SHA256" "$DEPS/bgfx.cmake"
fetch_archive zlib       "$ZLIB_URL"       "$ZLIB_SHA256"       "$WORK/zlib"
fetch_archive libpng     "$PNG_URL"        "$PNG_SHA256"        "$WORK/libpng"
fetch_archive sdl2       "$SDL_URL"        "$SDL_SHA256"        "$WORK/SDL2"

# Static -fPIC copies. The distro shared libraries would collide with the host game.
build_pic_deps() {
    if [[ -f "$PIC_PREFIX/lib/libSDL2.a" && -f "$PIC_PREFIX/lib/libpng16.a" && -f "$PIC_PREFIX/lib/libz.a" ]]; then
        log "static deps already built at $PIC_PREFIX"
        return
    fi
    log "building static zlib, libpng and SDL2"
    # zlib 1.2.11 and libpng 1.6.37 still declare CMake < 3.5, which CMake 4 rejects
    cmake -S "$WORK/zlib" -B "$WORK/zlib-build" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PIC_PREFIX" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    cmake --build "$WORK/zlib-build" --target install -j"$JOBS"

    cmake -S "$WORK/libpng" -B "$WORK/libpng-build" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PIC_PREFIX" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DPNG_SHARED=OFF -DPNG_TESTS=OFF -DPNG_TOOLS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DZLIB_INCLUDE_DIR="$PIC_PREFIX/include" -DZLIB_LIBRARY="$PIC_PREFIX/lib/libz.a"
    cmake --build "$WORK/libpng-build" --target install -j"$JOBS"

    # sdl 2.30.10 doesn't compile against pipewire 1.4 (pw_node_enum_params)
    cmake -S "$WORK/SDL2" -B "$WORK/sdl-build" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PIC_PREFIX" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_STATIC_PIC=ON \
        -DSDL_TEST=OFF -DSDL_TESTS=OFF -DSDL_INSTALL=ON \
        -DSDL_PIPEWIRE=OFF
    cmake --build "$WORK/sdl-build" --target install -j"$JOBS"

    for f in libSDL2.a libpng16.a libz.a; do
        [[ -f "$PIC_PREFIX/lib/$f" ]] || die "$f missing under $PIC_PREFIX/lib"
    done
}

copy_out() {
    [[ -f "$1" ]] || die "expected artifact $1 was not produced"
    mkdir -p "$OUT"
    cp -f "$1" "$OUT/$2"
    chmod +x "$OUT/$2"
    log "built $OUT/$2"
    sha256sum "$OUT/$2"
}

build_pic_deps
export PKG_CONFIG_PATH="$PIC_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

# one configure, bgfx bx and bimg get built once for every artifact
log "configuring $MODE"
cmake -S "$SRC/platform" -B "$WORK/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$PIC_PREFIX" \
    -DMG_DEPS="$DEPS" \
    -DMG_PIC_PREFIX="$PIC_PREFIX" \
    -DMG_TARGET="$MODE"
cmake --build "$WORK/build" -j"$JOBS"

if [[ "$MODE" == "exe" || "$MODE" == "both" ]]; then
    copy_out "$WORK/build/matrixgame" matrixgame
fi

if [[ "$MODE" == "dll" || "$MODE" == "both" ]]; then
    copy_out "$WORK/build/MatrixGame.so" MatrixGame.so

    log "verifying the library is self-contained"
    exported="$(nm -D --defined-only "$OUT/MatrixGame.so" | awk '$2=="T"||$2=="W"{print $3}' | grep -v '^\s*$' || true)"
    if [[ "$(echo "$exported" | grep -c .)" != "1" || "$exported" != "GetRobotInterface" ]]; then
        die "expected exactly one exported symbol (GetRobotInterface), got: $exported"
    fi
    if ldd "$OUT/MatrixGame.so" | grep -E "libSDL2|libpng|libz\.|libbgfx" >/dev/null; then
        die "MatrixGame.so still links a shared SDL2/png/zlib"
    fi
    log "exports: $exported"
fi

log "artifacts in $OUT"
log "run from the game root, DATA/ and CFG/ are opened relative to the current directory"
log "drop-in: put MatrixGame.so next to the host Rangers binary"
