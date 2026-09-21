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
BUILD_DIR="$(cd "${BUILD_DIR:-$SRC_DIR/build}" 2>/dev/null && pwd || echo "$SRC_DIR/build")"
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
QT_MAKESPEC="$($QMAKE -query QMAKE_SPEC)"

echo "== qmake      : $QMAKE"
echo "== qt libs    : $QT_LIBS"
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

    echo "== running make (parallel=$JOBS) =="
    make -j"$JOBS" || {
        echo ""
        echo "=== parallel build failed, retrying single-threaded ==="
        make -j1
    }

    if [ ! -x "$BUILD_DIR/flash_tool" ]; then
        echo ""
        echo "=== DIAGNOSTICS ==="
        echo "pwd: $(pwd)"
        echo "BUILD_DIR: $BUILD_DIR"
        echo "ls build/:"
        ls -la "$BUILD_DIR"/ 2>/dev/null | head -20
        echo ""
        echo "find flash_tool anywhere:"
        find "$SRC_DIR/build" "$SRC_DIR" -maxdepth 2 -name "flash_tool" -type f 2>/dev/null || true
        echo "=== END DIAGNOSTICS ==="
        exit 1
    fi

    echo "== build OK =="
    echo "== building patch_brom shim =="
    gcc -shared -fPIC -o "$BUILD_DIR/libpatch_brom.so" \
        "$SRC_DIR/lib/patch_brom.c" -ldl || {
        echo "warning: libpatch_brom.so build failed" >&2
    }
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

cp "$BUILD_DIR/flash_tool" "$STAGE/flash_tool"
chmod +x "$STAGE/flash_tool"

if [ -f "$SRC_DIR/lib/flash_tool.sh" ]; then
    cp "$SRC_DIR/lib/flash_tool.sh" "$STAGE/flash_tool.sh"
    chmod +x "$STAGE/flash_tool.sh"
fi

cp "$SRC_DIR"/lib/libflashtool.so* "$STAGE/lib/" 2>/dev/null || true
cp "$SRC_DIR"/lib/libflashtool.v1.so* "$STAGE/lib/" 2>/dev/null || true
cp "$SRC_DIR"/lib/libflashtoolEx.so* "$STAGE/lib/" 2>/dev/null || true
cp "$SRC_DIR"/lib/libsla_challenge.so* "$STAGE/lib/" 2>/dev/null || true

[ -f "$BUILD_DIR/libpatch_brom.so" ] && cp "$BUILD_DIR/libpatch_brom.so" "$STAGE/lib/"

for pat in '*.xml' '*.xsd' '*.ini' '*.bin' '*.json' '*.rules' '*.qhc' '*.qch' '*.sh' '*.conf'; do
    cp "$SRC_DIR"/lib/$pat "$STAGE/" 2>/dev/null || true
done
[ -f "$SRC_DIR/Rules/image_map.xml" ] && cp "$SRC_DIR/Rules/image_map.xml" "$STAGE/"

echo "== verifying binary =="
if ! ldd "$STAGE/flash_tool" >/dev/null 2>&1; then
    echo "warning: flash_tool has missing shared-library dependencies" >&2
    ldd "$STAGE/flash_tool" | grep 'not found' >&2 || true
fi

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
