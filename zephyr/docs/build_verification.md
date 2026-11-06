# Final verification record

All five applications were independently configured, compiled and linked for `frdm_mcxa153/mcxa153` with Zephyr 4.3.0 and ARM GNU 14.2.1. Final artifacts are in each application's **`build-verified/zephyr`** directory. Ignored `build` directories contain interrupted earlier attempts and are not deliverable firmware.

| Verification level | Result |
|---|---|
| Source review | Complete: ISR deferral, static storage, error paths, synchronization, command validation and test-mode labels reviewed |
| Configure/compile/link | PASS, all five |
| ARM GCC static analyzer | PASS, `-fanalyzer -Werror` on all five `main.c` and five `common.c` files with actual Zephyr board compile flags |
| Final Kconfig | PASS: expected board, UART shell, zero heap, stack sentinels and feature drivers |
| ELF/HEX/BIN consistency | PASS: all allocated ELF load sections match BIN; gaps match fill pattern; all HEX record checksums and data match BIN |
| Reset vectors | PASS offline: aligned initial SP in configured SRAM; reset handler is a Thumb address within image |
| Memory | PASS: all five within actual linker FLASH/RAM regions |
| Physical flash/board operation | **PENDING; not performed** |
| Runtime self-tests, stacks and measured timing | **PENDING; not executed on target** |

Machine-readable evidence: [build_audit.json](build_audit.json). Static analysis transcript: [static_analysis.txt](static_analysis.txt). Build-all script transcript: [build_all_verification.txt](build_all_verification.txt). Each ELF/HEX/BIN has a SHA-256 fingerprint in its individual memory report. The complete final linker map and compile command database remain inside the corresponding build directory.

Two known diagnostics are retained without hiding them:

- Zephyr emits `__ASSERT() statements are globally ENABLED`. Assertions are intentional for these demonstrator/test builds.
- GNU ld reports that the prelink and final ELF have a LOAD segment with RWX permissions. This comes from the standard Zephyr bare-metal link layout/flags. ELF program-header permissions are not an MPU protection configuration. The apps do not enable userspace or claim memory-isolation enforcement. No application C compiler warnings were present, and the separate analyzer treats warnings as errors.

The board-default ADC calibration dependency warning was removed by explicitly setting the unused calibration option to `n`. The official P3T1755 binding and I3C pinctrl remain in use.

Environment issues resolved during the work: Zephyr sources/dependencies were added in a separate workspace; the existing compiler is reused via a space-free directory junction; CMake 3.31.6 is isolated and pinned in the virtual environment; disk exhaustion was resolved by clearing generated diagnostics and duplicate download content from this task. No existing MCUXpresso project, SDK installation, board firmware or probe firmware was changed.

No build blockers remain. Physical button debounce, UART wiring/enumeration, actual sensor communication, timing distributions, watchdog recovery/reset and runtime stack margins still require the user to flash and observe the corresponding application. PuTTY should use the current MCU-Link VCOM port (COM23 requested), 115200 8-N-1, no flow control.

## Publication provenance

These results describe the supplied local builds in `C:\MCU_Expresso_Projects\FRDM-MCXA153_Zephyr_RTOS`. Publication copies application source and build configuration unchanged into this repository's `zephyr/` directory. The [publication manifest](publication_manifest.json) records their fingerprints; the [source audit](source_audit.json) records the original snapshot.

Build directories, linker maps, compile databases and generated binaries remain local and are excluded from Git. The retained build-all transcript is an incremental build/audit transcript, not a full compiler log. No hardware captures were available. Rebuilding at another path may change debug information and ELF hashes; compare actual build results and source provenance rather than assuming byte-identical ELFs.
