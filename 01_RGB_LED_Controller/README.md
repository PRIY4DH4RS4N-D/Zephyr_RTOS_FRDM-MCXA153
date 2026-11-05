# 01 · RGB LED Controller

GPIO driven colour sequence for the on-board LED of the NXP FRDM-MCXA153. [Repository overview](../README.md) · [Hardware](docs/hardware_configuration.md) · [Firmware](docs/firmware_architecture.md) · [Memory](docs/memory_report.md) · [Build environment](../docs/development_environment.md)

## What it does

After pin and clock initialization, the firmware sets GPIO3 pins 12, 13, and 0 as outputs and cycles through red, green, blue, yellow, cyan, magenta, white, and off indefinitely. A logical 0 turns a channel on. The program uses fixed GPIO states and blocking SDK delays; there is no PWM or runtime colour input.

| Step | R | G | B | Hold |
| --- | ---: | ---: | ---: | ---: |
| Red | 1 | 0 | 0 | 700 ms |
| Green | 0 | 1 | 0 | 700 ms |
| Blue | 0 | 0 | 1 | 700 ms |
| Yellow | 1 | 1 | 0 | 700 ms |
| Cyan | 0 | 1 | 1 | 700 ms |
| Magenta | 1 | 0 | 1 | 700 ms |
| White | 1 | 1 | 1 | 1 s |
| Off | 0 | 0 | 0 | 700 ms |

The R/G/B columns are arguments to `RGB_SetColor()`, not electrical output levels. For active-low channels, argument 1 writes level 0.

## Source layout

| Path | Purpose |
| --- | --- |
| `source/main.c` | RGB sequence and GPIO writes |
| `frdmmcxa153/board/pin_mux.*` | Generated mux setup for all three RGB pins |
| `frdmmcxa153/frdmmcxa153/` | Board and clock support used by CMake |
| `frdmmcxa153/led_blinky.mex` | MCUXpresso configuration source required by SDK CMake |
| `CMakeLists.txt`, `CMakePresets.json`, `prj.conf`, `Kconfig` | Reproducible SDK build configuration |

The supplied SDK contains the device startup, linker script, drivers, and remaining board integration. No generated build output belongs in Git.

## Build

Install MCUXpresso SDK 26.09, CMake, Ninja, Arm GNU toolchain, and the SDK Python environment. In PowerShell, from this directory:

```powershell
$env:SdkRootDirPath = '<directory containing mcuxsdk>'
$env:ARMGCC_DIR = '<Arm GNU toolchain directory>'
# Activate the SDK Python environment if the SDK setup requires it.
cmake --preset debug
cmake --build --preset debug
```

The resulting ELF is `debug/frdm_mcxa153_rgb_controller.elf`. The documented build was verified after packaging. Details for flashing and debugging are in [development_environment.md](../docs/development_environment.md).

## Board wiring

The built-in LED uses P3_12 for red, P3_13 for green, and P3_0 for blue, all on GPIO3. No external wiring is needed beyond the board's USB connection. The [hardware note](docs/hardware_configuration.md) explains initialization and the active-low levels.
