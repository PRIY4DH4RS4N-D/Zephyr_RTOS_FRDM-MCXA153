# Manual flashing and UART setup

No flashing, board reset or probe-firmware update was performed for these five Zephyr applications. Run a load command only when deliberately replacing the board's current firmware. Build and audit scripts do not invoke LinkServer.

## Checked LinkServer interface

The installed `C:\nxp\LinkServer_25.6.131\LinkServer.exe` was inspected using help, device-list and configuration-export operations. Its database identifies `MCXA153` with an empty board suffix. Use `MCXA153`; do not assume that the upstream Zephyr runner's `MCXA153:FRDM-MCXA153` pair works with this installation.

`flash --probe <serial> --update-mode none MCXA153 load <file>` accepts ELF, Intel HEX and SREC. ELF carries its addresses. Raw BIN requires the load address `0x00000000`. The load operation starts execution from reset unless `--no-reset` is used. `--update-mode none` avoids incidental probe-firmware updates.

## Program one demonstration at a time

From this repository's `zephyr` directory:

```powershell
$Link='C:\nxp\LinkServer_25.6.131\LinkServer.exe'
$Root=(Get-Location).Path
$Probe='EXHD3YASUKEFX' # Recorded development probe; replace for another board.

# Demonstration 1: execute this line, then finish its UART tests.
& $Link flash --probe $Probe --update-mode none MCXA153 load "$Root\01_rtos_message_pipeline\build-verified\zephyr\zephyr.elf"

# Demonstration 2: run separately after finishing demonstration 1.
& $Link flash --probe $Probe --update-mode none MCXA153 load "$Root\02_interrupt_button_fsm\build-verified\zephyr\zephyr.elf"

# Demonstration 3
& $Link flash --probe $Probe --update-mode none MCXA153 load "$Root\03_realtime_jitter_profiler\build-verified\zephyr\zephyr.elf"

# Demonstration 4
& $Link flash --probe $Probe --update-mode none MCXA153 load "$Root\04_onboard_thermal_monitor\build-verified\zephyr\zephyr.elf"

# Demonstration 5
& $Link flash --probe $Probe --update-mode none MCXA153 load "$Root\05_task_watchdog_supervisor\build-verified\zephyr\zephyr.elf"
```

Commands are prepared instructions, not programming evidence. Do not paste/run all five as a batch when demonstrating one image at a time. Close PuTTY before programming and reopen it afterwards. No mass erase option is required. To use HEX, substitute `zephyr.hex`; for BIN, use `zephyr.bin:0x00000000`. Check the device argument before using the upstream `west flash` runner on this host.

## PuTTY configuration

Select Serial and the current MCU-Link VCOM port. The recorded default is **COM23**, but enumeration is host-dependent. Set **115200 baud, 8 data bits, no parity, 1 stop bit and no flow control**. Use a VT100-compatible terminal, press Enter to submit commands, and enable session logging to capture real evidence.

Every image provides `help` and `resources`; application-specific commands and illustrative output formats are in each project README. Initial boot messages may be missed while opening the port. Static compilation/audit success does not verify UART wiring or console operation.

Only an explicit `guard fault <worker> reset` command requests the watchdog reset demonstration. Observe an actual reboot and CMC watchdog reset cause; the waiting message alone is insufficient. All physical checks remain pending.
