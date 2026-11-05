# Development environment and reproduction

## Validated toolchain

The project was developed with MCUXpresso SDK 26.09, VS Code, CMake, Ninja, Arm GNU GCC, NXP LinkServer 25.6.131, and the board's MCU-Link CMSIS-DAP interface. The packaged checkout was configured and compiled with CMake 4.3.2, Ninja 1.13.2, and Arm GCC 14.2.1 against the local MCUXpresso SDK. The SDK and tools must be installed separately.

## Configure and build

Open a terminal in `01_RGB_LED_Controller`. Set the SDK installation root containing `mcuxsdk`, and the Arm GNU toolchain directory. Make CMake and Ninja available on `PATH`. If the SDK supplies a Python virtual environment, activate it or put its scripts directory on `PATH` before configuring.

```powershell
$env:SdkRootDirPath = '<SDK installation directory>'
$env:ARMGCC_DIR = '<Arm GNU toolchain directory>'
cmake --preset debug
cmake --build --preset debug
```

The preset generates files under ignored `debug/`. Successful linking produces `debug/frdm_mcxa153_rgb_controller.elf`. The `release` preset can be built with the same commands after replacing `debug` with `release`; the reported memory snapshot is from a debug build. The project CMake file retains SDK-relative board integration and uses local board files from `frdmmcxa153/`.

## MCU-Link detection and flash

Connect the board's MCU-Link USB interface. With LinkServer on `PATH`, detect probes:

```powershell
LinkServer probes
```

The successful development setup identified board `FRDM-MCXA153`, MCU `MCXA153`, debugger `MCU-LINK`, interface `CMSIS-DAP`, serial `EXHD3YASUKEFX`, and virtual COM port `COM23`. Serial and COM port are host observations, not configuration requirements. With one probe connected, flash from the project directory:

```powershell
LinkServer flash MCXA153:FRDM-MCXA153 load debug/frdm_mcxa153_rgb_controller.elf
```

If multiple probes are attached, use LinkServer's `--probe` option with the desired probe index or serial supplied interactively. No probe serial is stored in the project scripts. The ELF is generated locally and is excluded from Git.

## Debugging

In VS Code with the MCUXpresso extension, open the project directory, select the `debug` CMake preset and the MCU-Link CMSIS-DAP probe, and launch a debug session against `debug/frdm_mcxa153_rgb_controller.elf`. Set a breakpoint in `main()` or `RGB_SetColor()` and step over the GPIO writes. For a command-line workflow, consult `LinkServer gdbserver --help` for the installed LinkServer version, start its GDB server for `MCXA153:FRDM-MCXA153`, and attach the Arm GNU `arm-none-eabi-gdb` to the locally built ELF. Exact server options can vary by installed tool version.

The firmware does not print to the virtual COM port. Visual LED states and breakpoints are the useful observations for this project.
