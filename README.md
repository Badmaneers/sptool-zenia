# SP Flash Tool for Linux v5.3-zenia

A Qt6-based flashing tool for MediaTek Android devices. Supports BROM mode, Download Agent mode, format, readback, memory test, firmware upgrade, and more.

## What's New in 5.3-zenia

- Qt6 Linux port with dark mode and modern UI
- BROM mode fix for Linux kernel 5.4+ (LD_PRELOAD shim)
- BROM mode fix for Linux kernel 7.0+ (broadened EOPNOTSUPP interception)
- UART as default connection type
- Native file dialogs
- Dark/Light theme toggle
- GitHub Actions CI/CD workflow
- Bundled xerces-c + ICU — no system install required
- **Payload tab** — crash device to BROM using mtk_payload before flashing

## Requirements

- Linux x86_64 (kernel 5.4+)
- **Qt6 runtime libraries** (Qt6Core, Qt6Widgets, Qt6Gui, Qt6Network, Qt6Core5Compat)

### Payload Tool Dependencies

The bundled payload tool (`payload/mtk_payload`) requires these system libraries:

| Library | Ubuntu/Debian | Arch |
|---------|---------------|------|
| libusb-1.0 | `libusb-1.0-0` | `libusb` |
| libgmp | `libgmp10` | `gmp` |
| libcrypto/libssl | `libssl3t64` | `openssl` |
| libudev | `libudev1` | `systemd-libs` |

Most are pre-installed on desktop systems. Install them if missing:

```bash
# Ubuntu/Debian
sudo apt install libusb-1.0-0 libgmp10 libssl3t64 libudev1

# Arch
sudo pacman -S libusb gmp openssl systemd-libs
```

### Full Install (all dependencies)

```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev libqt6core5compat6 \
    libusb-1.0-0 libgmp10 libssl3t64 libudev1

# Arch
sudo pacman -S qt6-base qt6-5compat libusb gmp openssl systemd-libs
```

## Installation

1. Extract the release archive:

```bash
unzip flash_tool_linux_5.3-zenia.zip
cd flash_tool_linux_v5.3-zenia
```

2. Install udev rules so the tool can access USB devices without root:

```bash
sudo cp 99-ttyacms.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

3. Launch the tool:

```bash
./flash_tool.sh
```

Or run the binary directly (auto-loads the BROM shim):

```bash
./flash_tool
```

## First Launch

- The tool starts in **Dark Mode** with **UART** as the default connection type.
- To change theme or connection settings, go to **Options > USB and UART options**.
- If you don't see any COM ports listed, ensure the udev rules are installed and your user is in the `plugdev` group:

```bash
sudo usermod -aG plugdev $USER
```

Log out and back in for the group change to take effect.

## Payload Tab (Crash to BROM)

For locked or security-enabled devices, use the **Payload** tab before flashing:

1. Connect the device via USB.
2. Go to the **Payload** tab (next to Download).
3. Select the payload type (or leave as Auto for Kamakiri2).
4. Click **Run Payload** — the tool exploits the chip and crashes it into BROM mode.
5. Once the status shows "Done — device in BROM, switch to Download tab", go to the **Download** tab and flash as normal.

The payload tool auto-detects the connected MediaTek chip and resolves the correct exploit payload. Supported payload types: Kamakiri, Kamakiri2, Amonet, Hashimoto, Carbonara.

## Connecting a Device

1. Power off the device completely.
2. Connect the device via USB.
3. In the tool, select the correct COM port (for UART) or use USB mode.
4. Click **Connect**.

For BROM mode (preloader/bootrom), the tool automatically detects the device when it enters BROM mode. On kernel 5.4+, a built-in shim handles the CDC ACM ioctl compatibility — no manual configuration needed.

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `Could not find Qt platform plugin` | Install Qt6 runtime: `sudo apt install qt6-base-dev` |
| `Connect BROM failed: STATUS_ERR` | Ensure device is powered off and connected via USB. Check udev rules are installed. |
| `mtk_payload binary not found` | Ensure the `payload/` directory is bundled in the dist (rebuild with `./build.sh`). |
| No COM ports listed | Install udev rules, add user to `plugdev` group, re-login. |
| Permission denied on `/dev/ttyACM*` | Run `sudo chmod 666 /dev/ttyACM*` or reinstall udev rules. |
| Dark theme looks wrong | Go to Options > Appearance and select your preferred theme. |
| Tool doesn't start | Run `./flash_tool` from terminal and check error output. |

## Building from Source

See [BUILD.md](BUILD.md) for prerequisites, build options, and architecture details.

```bash
./build.sh
```

## Supported Platforms

MT6573, MT6575, MT6577, MT6589, MT6572, MT6582, MT8135, MT6592, MT6571, MT8127, MT6595, MT6752, MT2601, MT8173, MT6795, MT6735, MT6753, MT8163, MT8590, MT6580, MT7623, MT7683

## File Structure

```
flash_tool_linux_v5.3-zenia/
├── flash_tool              # Main binary
├── flash_tool.sh           # Launcher script (recommended)
├── 99-ttyacms.rules        # udev rules for USB device access
├── MTK_AllInOne_DA.bin     # Download Agent binary
├── DA_PL.bin               # Preloader DA
├── DA_SWSEC.bin            # Secure DA
├── option.ini              # Saved settings
├── usb_setting.xml         # USB device ID definitions
├── platform.xml            # Platform definitions
├── lib/                    # MTK libraries + BROM shim + xerces-c
├── payload/                # mtk_payload tool + chip exploit binaries
│   ├── mtk_payload         # BROM crash/exploit binary (libusb)
│   └── payloads/           # Per-chip payload .bin files
├── flashtool.qhc           # Qt Assistant help collection
└── flashtool.qch           # Qt Assistant help data
```
