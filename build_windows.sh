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
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-devel-${SDL_VERSION}-mingw.tar.gz"
SDL_SHA256="a7763f9439ea25685b053e9257dac1eac012e5cd0824f1a801b27b1d92ebe321"
ZLIB_URL="https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz"
ZLIB_SHA256="629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff"
PNG_URL="https://github.com/glennrp/libpng/archive/refs/tags/v1.6.37.tar.gz"
PNG_SHA256="ca74a0dace179a8422187671aee97dd3892b53e168627145271cad5b5ac81307"

MINGW_TRIPLET="x86_64-w64-mingw32"
SRC="${MG_SRC:-$SCRIPT_DIR}"
WORK="${MG_WORK:-$SCRIPT_DIR/build/windows}"
DEPS="$WORK/deps"
PREFIX="$WORK/win-libs"
CACHE="$WORK/cache"
SDL_ROOT="$WORK/SDL2-$SDL_VERSION"
ARCH="${MG_ARCH:-x86_64}"
OUT="${MG_OUT:-$SCRIPT_DIR/out}/windows/$ARCH"

MODE="${MG_MODE:-both}"
die() { echo "build_windows: $*" >&2; exit 1; }
log() { echo "==> $*"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --mode=*) MODE="${1#*=}" ;;
        --mode) shift; MODE="${1:?--mode needs exe, dll or both}" ;;
        -h|--help) sed -n '2,18p' "${BASH_SOURCE[0]}"; exit 0 ;;
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

pick_posix_cxx() {
    if command -v "${MINGW_TRIPLET}-g++-posix" >/dev/null 2>&1; then
        echo "${MINGW_TRIPLET}-g++-posix"
    elif command -v "${MINGW_TRIPLET}-g++" >/dev/null 2>&1; then
        echo "${MINGW_TRIPLET}-g++"
    else
        return 1
    fi
}
CXX_BIN="$(pick_posix_cxx)" || die "no ${MINGW_TRIPLET}-g++ found. apt install g++-mingw-w64-x86-64"
log "cross compiler: $CXX_BIN"

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
[[ -f "$SRC/cmake/mingw64-toolchain.cmake" ]] || die "missing $SRC/cmake/mingw64-toolchain.cmake"

mkdir -p "$WORK"
JOBS="$(jobs_count)"
TOOLCHAIN="$SRC/cmake/mingw64-toolchain.cmake"
log "source $SRC  mode=$MODE  jobs=$JOBS"

fetch_archive bx         "$BX_URL"         "$BX_SHA256"         "$DEPS/bx"
fetch_archive bimg       "$BIMG_URL"       "$BIMG_SHA256"       "$DEPS/bimg"
fetch_archive bgfx       "$BGFX_URL"       "$BGFX_SHA256"       "$DEPS/bgfx"
fetch_archive bgfx.cmake "$BGFX_CMAKE_URL" "$BGFX_CMAKE_SHA256" "$DEPS/bgfx.cmake"
fetch_archive sdl2-mingw "$SDL_URL"        "$SDL_SHA256"        "$SDL_ROOT"
fetch_archive zlib       "$ZLIB_URL"       "$ZLIB_SHA256"       "$WORK/zlib"
fetch_archive libpng     "$PNG_URL"        "$PNG_SHA256"        "$WORK/libpng"

[[ -f "$SDL_ROOT/$MINGW_TRIPLET/lib/libSDL2.dll.a" ]] || die "SDL2 MinGW tree incomplete at $SDL_ROOT"
[[ "$MODE" == "exe" || -f "$SDL_ROOT/$MINGW_TRIPLET/lib/libSDL2.a" ]] ||
    die "dll mode needs $SDL_ROOT/$MINGW_TRIPLET/lib/libSDL2.a"

if [[ ! -f "$PREFIX/lib/libpng16.a" || ! -f "$PREFIX/lib/libzlibstatic.a" ]]; then
    log "cross-building zlib and libpng"
    # zlib 1.2.11 and libpng 1.6.37 still declare CMake < 3.5, which CMake 4 rejects
    cmake -S "$WORK/zlib" -B "$WORK/zlib-build" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DBUILD_SHARED_LIBS=OFF
    cmake --build "$WORK/zlib-build" --target install -j"$JOBS"

    cmake -S "$WORK/libpng" -B "$WORK/libpng-build" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DPNG_SHARED=OFF -DPNG_TESTS=OFF -DPNG_BUILD_ZLIB=OFF \
        -DZLIB_INCLUDE_DIR="$PREFIX/include" \
        -DZLIB_LIBRARY="$PREFIX/lib/libzlibstatic.a"
    cmake --build "$WORK/libpng-build" --target install -j"$JOBS"
fi
[[ -f "$PREFIX/lib/libpng16.a" ]] || die "libpng16.a missing under $PREFIX/lib"
PNG_A="$(find "$PREFIX" -name 'libpng16.a' | head -1)"
ZLIB_A="$PREFIX/lib/libzlibstatic.a"
[[ -f "$ZLIB_A" ]] || ZLIB_A="$(find "$PREFIX" -name 'libz.a' | head -1)"
[[ -f "$ZLIB_A" ]] || die "static zlib missing under $PREFIX"

configure_common() {
    cmake -S "$SRC/platform" -B "$1" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release \
        -DMG_DEPS="$DEPS" \
        -DMG_SDL2_ROOT="$SDL_ROOT" \
        -DMG_PNG_INCLUDE="$PREFIX/include" \
        -DMG_PNG_LIBRARY="$PNG_A" \
        -DMG_ZLIB_INCLUDE="$PREFIX/include" \
        -DMG_ZLIB_LIBRARY="$ZLIB_A" \
        -DMG_TARGET="$2"
}

copy_out() {
    [[ -f "$1" ]] || die "expected artifact $1 was not produced"
    mkdir -p "$OUT"
    cp -f "$1" "$OUT/$2"
    log "built $OUT/$2"
    sha256sum "$OUT/$2"
}

if [[ "$MODE" == "exe" || "$MODE" == "both" ]]; then
    log "configuring executable"
    configure_common "$WORK/build-exe" exe
    cmake --build "$WORK/build-exe" -j"$JOBS"
    copy_out "$WORK/build-exe/matrixgame.exe" matrixgame.exe
    cp -f "$SDL_ROOT/$MINGW_TRIPLET/bin/SDL2.dll" "$OUT/SDL2.dll"
    pthread_dll="$("$CXX_BIN" -print-file-name=libwinpthread-1.dll 2>/dev/null || true)"
    if [[ -f "$pthread_dll" ]]; then
        cp -f "$pthread_dll" "$OUT/libwinpthread-1.dll"
    else
        log "warning: libwinpthread-1.dll not found via $CXX_BIN"
    fi
fi

if [[ "$MODE" == "dll" || "$MODE" == "both" ]]; then
    log "configuring MatrixGame.dll"
    configure_common "$WORK/build-dll" dll
    cmake --build "$WORK/build-dll" -j"$JOBS"
    copy_out "$WORK/build-dll/MatrixGame.dll" MatrixGame.dll

    log "verifying the dll is self-contained"
    deps="$(x86_64-w64-mingw32-objdump -p "$OUT/MatrixGame.dll" | awk '/^[[:space:]]+DLL Name:/{print $3}')"
    [[ -n "$deps" ]] || die "could not read the import table of MatrixGame.dll"
    if echo "$deps" | grep -iE "SDL2|libpng|zlib|winpthread|stdc|gcc_s" >/dev/null; then
        echo "$deps" >&2
        die "MatrixGame.dll still imports SDL2/zlib/png/winpthreads"
    fi
    log "dll imports only system dlls: $(echo "$deps" | tr '\n' ' ')"
    exports="$(x86_64-w64-mingw32-objdump -p "$OUT/MatrixGame.dll" |
        sed -n '/Ordinal\/Name Pointer/,$p' |
        sed -n -E 's/^[[:space:]]*\[[[:space:]]*[0-9]+\][[:space:]]+([A-Za-z_][A-Za-z0-9_]*)$/\1/p')"
    echo "$exports" | grep -qx "GetRobotInterface" ||
        die "MatrixGame.dll does not export GetRobotInterface"
    log "dll exports: $(echo "$exports" | tr '\n' ' ')"
fi

log "artifacts in $OUT"
log "the exe loads DATA/ and CFG/ from the current directory; keep SDL2.dll beside it"
log "drop-in: put MatrixGame.dll next to the host Rangers.exe"
