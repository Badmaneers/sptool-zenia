# Building SP Flash Tool v5.3-zenia from Source

## Prerequisites

### Qt6 Development Libraries

```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-5compat-dev qt6-tools-dev \
    libgl-dev libx11-dev

# Arch
sudo pacman -S qt6-base qt6-5compat
```

### Build Tools

```bash
# Ubuntu/Debian
sudo apt install build-essential g++ qmake6 patchelf zip

# Arch
sudo pacman -S base-devel gcc qmake6 patchelf zip
```

### Xerces-C (XML parser)

```bash
# Ubuntu/Debian
sudo apt install libxerces-c-dev

# Arch
sudo pacman -S xerces-c
```

## Quick Build

```bash
./build.sh
```

This will:
1. Run `qmake6` to generate the Makefile
2. Compile the binary (`flash_tool`)
3. Build the BROM shim (`libpatch_brom.so`)
4. Stage all runtime libraries and data files into `dist/flash_tool_linux_<date>/`
5. Package as a `.zip` (or `.tar.gz` if zip isn't installed)

Output: `dist/flash_tool_linux_<date>.zip`

## Build Options

```bash
./build.sh                    # Full build + package (default)
./build.sh my_release         # Custom output name
./build.sh --make-only        # Build binary only, no packaging
./build.sh --clean            # Clean build dir, rebuild + package
./build.sh --clean my_release # Clean + custom name
```

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `BUILD_DIR` | `./build` | Out-of-tree build directory |
| `JOBS` | `$(nproc)` | Number of parallel make jobs |

## CI/CD

The project includes a GitHub Actions workflow (`.github/workflows/build.yml`) that:
- Builds on every push to `main` and on pull requests
- Creates a GitHub release when a `v*` tag is pushed
- Supports manual triggering with a release toggle via `workflow_dispatch`

## Manual Build Steps

If you prefer to build step by step:

### 1. Run qmake

```bash
mkdir -p build && cd build
qmake6 ../SPFlashToolAPCore.pro CONFIG+=release
```

### 2. Compile

```bash
make -j$(nproc)
```

The binary is produced at `build/flash_tool`.

### 3. Build the BROM Shim

```bash
gcc -shared -fPIC -o build/libpatch_brom.so lib/patch_brom.c -ldl
```

This shim intercepts `TIOCGSERIAL`/`TIOCSSERIAL` ioctls that kernel 5.4+ CDC ACM drivers reject, allowing BROM mode to work on modern kernels.

### 4. Stage the Distribution

```bash
NAME="flash_tool_linux_$(date +%Y%m%d)"
STAGE="dist/$NAME"
mkdir -p "$STAGE/lib"

# Binary
cp build/flash_tool "$STAGE/"
chmod +x "$STAGE/flash_tool"

# Launcher
cp lib/flash_tool.sh "$STAGE/"
chmod +x "$STAGE/flash_tool.sh"

# MTK libraries
cp lib/libflashtool.so* "$STAGE/lib/"
cp lib/libflashtool.v1.so* "$STAGE/lib/"
cp lib/libflashtoolEx.so* "$STAGE/lib/"
cp lib/libsla_challenge.so* "$STAGE/lib/"

# BROM shim
cp build/libpatch_brom.so "$STAGE/lib/"

# Data files
cp lib/*.xml lib/*.xsd lib/*.bin lib/*.json lib/*.rules \
   lib/*.ini lib/*.conf lib/*.sh "$STAGE/" 2>/dev/null || true

# Qt6 runtime libraries (see build.sh for full list)
# The build script auto-resolves and copies all Qt6 and xerces dependencies.
```

## Architecture

### BROM Shim (`lib/patch_brom.c`)

On Linux kernel 5.4+, the CDC ACM USB serial driver returns `EOPNOTSUPP` for `TIOCGSERIAL` and `TIOCSSERIAL` ioctls. The precompiled MTK library treats this as a fatal error during BROM connection.

The shim (`libpatch_brom.so`) is loaded via `LD_PRELOAD` by the launcher script, or automatically by the binary itself via self-reexec. It intercepts these ioctls on `/dev/ttyACM*` devices and returns fake success.

### Self-Reexec (`main.cpp`)

When `flash_tool` is run directly (not via `flash_tool.sh`), it checks at startup whether the BROM shim is loaded. If not, it sets `LD_PRELOAD` and re-executes itself. This ensures the shim is always active regardless of how the binary is launched.

### Theme System

The tool supports multiple themes via Qt stylesheets:
- **Dark Mode** (default) — VS Code-inspired dark theme
- **Modern** — Light flat theme
- **Fusion** — Qt Fusion style
- **Classic** — Background image theme
- **Custom** — User-provided QSS file

Themes are applied at startup from `style.qss` or `style-dark.qss` embedded in the Qt resource system.

## RPATH Configuration

The binary uses `$ORIGIN` and `$ORIGIN/lib` as RPATH with `--disable-new-dtags` (DT_RPATH instead of DT_RUNPATH). This ensures the precompiled MTK libraries can find Qt6 libraries in the bundled `lib/` directory without requiring system-wide installation.

## Troubleshooting the Build

| Error | Solution |
|-------|----------|
| `no Qt6 qmake found` | Install `qt6-base-dev` or ensure `qmake6` is in your PATH |
| `libxerces-c not found` | Install `libxerces-c-dev` |
| `libpatch_brom.so build failed` | Ensure `gcc` is installed (build-essential) |
| `unresolved dependencies` after build | Check `ldd build/flash_tool` for missing libs, install them |
