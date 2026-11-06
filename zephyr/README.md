# FRDM-MCXA153 Zephyr applications

This directory contains five independent Zephyr 4.3.0 applications. The [repository overview](../README.md) provides hardware specifications, architecture, tool versions and the consolidated memory table. The existing MCUXpresso SDK project remains outside this directory.

| ID | Application | Primary command |
|---|---|---|
| P01 | [RTOS Message Pipeline](01_rtos_message_pipeline/README.md) | `pipeline inject 123` |
| P02 | [Interrupt-Driven Button FSM](02_interrupt_button_fsm/README.md) | `fsm status` |
| P03 | [Real-Time Jitter Profiler](03_realtime_jitter_profiler/README.md) | `bench report` |
| P04 | [Onboard Thermal Monitor](04_onboard_thermal_monitor/README.md) | `thermal read` |
| P05 | [Task Watchdog Supervisor](05_task_watchdog_supervisor/README.md) | `guard status` |

Each numbered directory contains `CMakeLists.txt`, `Kconfig`, `prj.conf`, `app.overlay`, `sample.yaml`, `src/main.c`, local helpers, a README and static memory reports. Build/configuration/firmware files match the supplied verified workspace; see the [publication manifest](docs/publication_manifest.json).

## Build and verify

From this directory, after [environment setup](docs/zephyr_installation.md):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_all.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_project.ps1 -Project 04_onboard_thermal_monitor -Pristine
```

See [BUILD_GUIDE.md](BUILD_GUIDE.md), [memory comparison](docs/memory_comparison.md), [verification record](docs/build_verification.md) and [manual flashing guide](docs/flashing_guide.md). Outputs are generated locally under each application's ignored `build-verified/zephyr/` directory. No build trees, linker maps or firmware binaries are committed.

All five original builds, offline artifact checks and application static-analysis checks passed. Runtime tests and physical operation remain **PENDING**. No hardware screenshots or UART captures were available. The retained [build-all transcript](docs/build_all_verification.txt) and [static-analysis transcript](docs/static_analysis.txt) are build evidence only.

Run publication checks without any hardware or Zephyr installation:

```powershell
python .\scripts\verify_publication.py
```

On the original development host, add `--source-workspace C:\MCU_Expresso_Projects\FRDM-MCXA153_Zephyr_RTOS` to compare supplied source and artifact fingerprints as well. The [root BSD 3-Clause license](../LICENSE) and [third-party notices](../THIRD_PARTY_NOTICES.md) apply.
