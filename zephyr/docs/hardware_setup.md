# Hardware configuration and evidence

Verified against the pinned Zephyr 4.3.0 files under `C:\zephyr-work\zephyr`:

| Peripheral | Official configuration | Use |
|---|---|---|
| Board | `boards/nxp/frdm_mcxa153/board.yml` | `frdm_mcxa153`, SoC qualifier `mcxa153` |
| Red LED | alias `led0`, GPIO3 pin 12, active low | RGB mask bit 0 |
| Green LED | alias `led1`, GPIO3 pin 13, active low | RGB mask bit 1 |
| Blue LED | alias `led2`, GPIO3 pin 0, active low | RGB mask bit 2 |
| SW2 | alias `sw0`, GPIO3 pin 29, active low | FSM cycle |
| SW3 | alias `sw1`, GPIO1 pin 7, active low | FSM configure/acknowledge |
| Shell and console | chosen `lpuart0`, RX P0.2, TX P0.3 | MCU-Link VCOM, 115200 8-N-1 |
| P3T1755 | alias `ambient-temp0`, `nxp,p3t1755` on `i3c0` | Static address 0x48, PID 0x0236152a0090 |
| I3C0 pins | SDA P0.16, SCL P0.17, PUR P1.11 | Official pinctrl including pull-ups |
| WWDT | alias `watchdog0`, `nxp,lpc-wwdt` | Task Watchdog hardware fallback |
| Reset cause | `nxp,cmc-reset-cause` | `hwinfo_get_reset_cause()` |

GPIO operations use `gpio_dt_spec` and logical active/inactive values; the driver applies polarity. Button callbacks perform only atomic updates and delayed-work rescheduling. No UART, sleeps or mutex acquisition occurs in the GPIO ISR.

Evidence files: `boards/nxp/frdm_mcxa153/frdm_mcxa153.dts`, `frdm_mcxa153-pinctrl.dtsi`, `dts/arm/nxp/nxp_mcxa153.dtsi`, `drivers/sensor/nxp/p3t1755/p3t1755.c`, `drivers/watchdog/wdt_mcux_wwdt.c`, `drivers/hwinfo/hwinfo_mcux_mcx_cmc.c`. Final effective settings are preserved in each build's `zephyr/zephyr.dts` and `.config`.

The SoC runs at 96 MHz. The official board selects the OS timer for kernel time; the profiler separately uses the Cortex-M timing backend. Linker SRAM is 24 KiB at 0x20000000. Do not add other memory banks to the free-memory number without changing and validating the linker configuration.

Connect the onboard MCU-Link USB to the PC when ready to test. No external components are required. Select the enumerated VCOM port in PuTTY. COM23 is the requested default but was not detectable during the initial audit; no port was opened and no board operation was performed.

References: [official Zephyr board documentation](https://docs.zephyrproject.org/4.3.0/boards/nxp/frdm_mcxa153/doc/index.html), [pinned board source](https://github.com/zephyrproject-rtos/zephyr/blob/v4.3.0/boards/nxp/frdm_mcxa153/frdm_mcxa153.dts).
