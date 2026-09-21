#!/usr/bin/env bash
#
# build.sh -- build SP Flash Tool against Qt6 and package a ready-to-ship zip.
#
# Usage:
#   ./build.sh                     full build + zip (name: flash_tool_linux_<date>)
#   ./build.sh my_release          full build + zip with a custom name
#   ./build.sh --make-only         only build the binary, no packaging
#   ./build.sh --clean [name]      clean + full rebuild + zip
#
# Outputs:
#   build/   out-of-tree build directory (qmake/make)
#   dist/    staging directory + final zip

set -euo pipefail

# ---------------------------------------------------------------------------
# options
# ---------------------------------------------------------------------------
DO_PACKAGE=1
DO_CLEAN=0
NAME=""

for arg in "$@"; do
    case "$arg" in
        --make-only) DO_PACKAGE=0 ;;
        --clean)     DO_CLEAN=1 ;;
        --help|-h)
            grep '^#' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            if [ -z "$NAME" ]; then
                NAME="$arg"
            else
                echo "error: too many arguments" >&2
                exit 1
            fi
            ;;
    esac
done

# ---------------------------------------------------------------------------
# paths
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
SRC_DIR="$SCRIPT_DIR"
BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build}"
DIST_DIR="$SRC_DIR/dist"

# ---------------------------------------------------------------------------
# toolchain discovery
# ---------------------------------------------------------------------------
QMAKE=""
if command -v qmake6 >/dev/null 2>&1; then
    QMAKE="$(command -v qmake6)"
elif command -v qmake >/dev/null 2>&1; then
    CANDIDATE="$(command -v qmake)"
    QT_VERSION="$($CANDIDATE -query QT_VERSION 2>/dev/null || true)"
    case "$QT_VERSION" in
        6.*) QMAKE="$CANDIDATE" ;;
        *)   QMAKE="" ;;
    esac
fi

if [ -z "$QMAKE" ]; then
    echo "error: no Qt6 qmake found (looked for qmake6 / a Qt6 'qmake')" >&2
    exit 1
fi

QT_LIBS="$($QMAKE -query QT_INSTALL_LIBS)"
QT_PLUGINS="$($QMAKE -query QT_INSTALL_PLUGINS)"
QT_MAKESPEC="$($QMAKE -query QMAKE_SPEC)"

echo "== qmake      : $QMAKE"
echo "== qt libs    : $QT_LIBS"
echo "== qt plugins : $QT_PLUGINS"
echo "== mkspec     : $QT_MAKESPEC"
echo "== build dir  : $BUILD_DIR"

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

# ---------------------------------------------------------------------------
# build
# ---------------------------------------------------------------------------
build() {
    echo "== building (jobs=$JOBS) =="

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    "$QMAKE" "$SRC_DIR/SPFlashToolAPCore.pro" "CONFIG+=release"
    make -j"$JOBS"

    if [ ! -x "$BUILD_DIR/flash_tool" ]; then
        echo "error: $BUILD_DIR/flash_tool not produced" >&2
        exit 1
    fi

    if ! ldd "$BUILD_DIR/flash_tool" >/dev/null 2>&1; then
        echo "error: flash_tool has missing shared-library dependencies (see ldd)" >&2
        ldd "$BUILD_DIR/flash_tool" | grep 'not found' >&2 || true
        exit 1
    fi

    echo "== build OK =="

    # --- build shim for BROM on kernel >= 5.4 --------------------------
    echo "== building patch_brom shim =="
    gcc -shared -fPIC -o "$BUILD_DIR/libpatch_brom.so" \
        "$SRC_DIR/lib/patch_brom.c" -ldl
    if [ ! -f "$BUILD_DIR/libpatch_brom.so" ]; then
        echo "warning: libpatch_brom.so build failed" >&2
    fi
}

if [ "$DO_CLEAN" = "1" ]; then
    rm -rf "$BUILD_DIR"
fi
build

if [ "$DO_PACKAGE" = "0" ]; then
    echo "== done (make-only, no packaging): $BUILD_DIR/flash_tool =="
    exit 0
fi

# ---------------------------------------------------------------------------
# packaging
# ---------------------------------------------------------------------------
if [ -z "$NAME" ]; then
    NAME="flash_tool_linux_$(date +%Y%m%d)"
fi

STAGE="$DIST_DIR/$NAME"
rm -rf "$STAGE"
mkdir -p "$STAGE/lib"

# --- the binary ----------------------------------------------------------
cp "$BUILD_DIR/flash_tool" "$STAGE/flash_tool"
chmod +x "$STAGE/flash_tool"

# --- launcher -------------------------------------------------------------
if [ -f "$SRC_DIR/lib/flash_tool.sh" ]; then
    cp "$SRC_DIR/lib/flash_tool.sh" "$STAGE/flash_tool.sh"
    chmod +x "$STAGE/flash_tool.sh"
fi

# --- prebuilt Mediatek libraries (uppercase not used: all lowercase now) --
cp "$SRC_DIR"/lib/libflashtool.so* "$STAGE/lib/" 2>/dev/null || true
cp "$SRC_DIR"/lib/libflashtool.v1.so* "$STAGE/lib/" 2>/dev/null || true
cp "$SRC_DIR"/lib/libflashtoolEx.so* "$STAGE/lib/" 2>/dev/null || true
cp "$SRC_DIR"/lib/libsla_challenge.so* "$STAGE/lib/" 2>/dev/null || true

# --- LD_PRELOAD shim for BROM on kernel >= 5.4 --------------------------
[ -f "$BUILD_DIR/libpatch_brom.so" ] && cp "$BUILD_DIR/libpatch_brom.so" "$STAGE/lib/"

# --- data files (mirrors the original deployment set) ---------------------
for pat in '*.xml' '*.xsd' '*.ini' '*.bin' '*.json' '*.rules' '*.qhc' '*.qch' '*.sh' '*.conf'; do
    cp "$SRC_DIR"/lib/$pat "$STAGE/" 2>/dev/null || true
done
[ -f "$SRC_DIR/Rules/image_map.xml" ] && cp "$SRC_DIR/Rules/image_map.xml" "$STAGE/"

# --- Qt6 / xerces runtime libraries --------------------------------------
# Collect every NEEDED lib named libQt6* / libxerces* from the binary and
# from every bundled plugin, then copy it (and its symlink) into lib/.
needed_of() {
    objdump -p "$1" 2>/dev/null | awk '/NEEDED/ && $2 ~ /^libQt6/ || (/NEEDED/ && $2 ~ /^libxerces/){print $2}'
}

resolve_lib() {
    local name="$1"
    if [ -e "$QT_LIBS/$name" ]; then
        echo "$QT_LIBS/$name"
        return
    fi
    ldconfig -p 2>/dev/null | awk -v n="$name" '$1==n {print $NF; exit}' 
}

copy_runtime_lib() {
    local name="$1"
    if [ -e "$STAGE/lib/$name" ]; then
        return
    fi
    local src
    src="$(resolve_lib "$name")"
    if [ -z "$src" ] || [ ! -e "$src" ]; then
        echo "warning: could not resolve $name" >&2
        return
    fi
    cp -dP "$src" "$STAGE/lib/"
    local real
    real="$(readlink -f "$src")"
    if [ "$real" != "$src" ] && [ ! -e "$STAGE/lib/$(basename "$real")" ]; then
        cp -dP "$real" "$STAGE/lib/"
    fi
}

# 1) dependencies of the binary itself
while IFS= read -r dep; do
    [ -n "$dep" ] && copy_runtime_lib "$dep"
done < <(needed_of "$BUILD_DIR/flash_tool")

# 2) Qt plugins
PLUGIN_CATEGORIES="platforms platforminputcontexts imageformats iconengines \
                   generic xcbglintegrations egldeviceintegrations tls"
for cat in $PLUGIN_CATEGORIES; do
    dst="$STAGE/lib/$cat"
    mkdir -p "$dst"
    for plug in "$QT_PLUGINS"/$cat/libq*.so; do
        [ -e "$plug" ] || continue
        cp "$plug" "$dst/"
        while IFS= read -r dep; do
            [ -n "$dep" ] && copy_runtime_lib "$dep"
        done < <(needed_of "$plug")
    done
done

# --- sanity: all bundled runtime deps must resolve from staged lib/ -------
echo "== verifying staged dependencies =="
LD_LIBRARY_PATH="$STAGE/lib" ldd "$STAGE/flash_tool" | grep 'not found' && {
    echo "error: staged flash_tool still has unresolved dependencies" >&2
    exit 1
}

# --- assemble zip ---------------------------------------------------------
cd "$DIST_DIR"
if command -v zip >/dev/null 2>&1; then
    rm -f "$NAME.zip"
    zip -r -q "$NAME.zip" "$NAME"
    echo "== packaged: $DIST_DIR/$NAME.zip =="
else
    tar -czf "$NAME.tar.gz" "$NAME"
    echo "== 'zip' not found, packaged instead: $DIST_DIR/$NAME.tar.gz =="
fi

echo "== done =="