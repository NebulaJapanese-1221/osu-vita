# osu! vita

Unofficial recreation of osu! for the PlayStation Vita. This is not a port.

No osu! account required. Transfer beatmaps from your computer via FTP or USB.

osu! vita is not affiliated with the real osu! or ppy.

## Prerequisites

- PS Vita with custom firmware (3.00+)
- [VitaSDK](https://vitasdk.org/) installed and `VITASDK` environment variable set
- CMake 3.16+
- make / ninja

## Building

```bash
./build.sh
```

To clean build artifacts:
```bash
./build.sh clean
```

The VPK will be output to `build/osu_vita.vpk`.

## Installation

1. Transfer `build/osu_vita.vpk` to `ux0:/vpk/` via FTP or USB
2. In VitaShell, highlight the VPK and press `Cross` to install
3. Launch from the LiveArea

## Adding Beatmaps

1. On your computer, download `.osu` beatmap files
2. Transfer them to `ux0:/data/osu-vita/maps/` via FTP or USB
3. In osu! vita, select **Play** from the main menu to browse and play

## Requirements

- Enable **Unsafe Homebrew** in Henkaku Settings
- `ux0:/data/osu-vita/maps/` must contain `.osu` files
