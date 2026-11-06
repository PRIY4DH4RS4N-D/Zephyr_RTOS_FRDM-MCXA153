# Contributing

Keep each firmware application independently buildable and keep the Zephyr and MCUXpresso SDK workflows separate. Numbered Zephyr applications live under `zephyr/`; the existing `01_RGB_LED_Controller/` remains an SDK project.

## Development workflow

1. Create a topic branch from current `main` and keep changes focused.
2. Read the affected README, source, `prj.conf` and overlay before modifying behavior. Use official Zephyr APIs and the pinned board definitions.
3. Build the affected application with `zephyr/scripts/build_project.ps1`. Build all five when changing common code or shared scripts.
4. Audit generated artifacts. On Windows, `zephyr/scripts/analyze_sources.py` uses each real `compile_commands.json` with ARM GCC `-fanalyzer -Werror`.
5. Update commands, diagrams, memory reports and evidence. Run `zephyr/scripts/verify_publication.py`. If firmware intentionally changes, regenerate the provenance snapshot after reviewing the new build evidence.
6. Review `git diff --check`, staged paths and the staged diff. Exclude build trees, generated binaries, IDE state and credentials. Submit a pull request explaining behavior, validation and remaining physical tests.

Use meaningful commit messages, such as `feat(thermal): add configurable sample period`. Do not force-push shared branches or discard unrelated changes.

## Firmware expectations

- Keep ISR callbacks bounded: publish flags/events and defer processing. Do not sleep, perform blocking I/O or acquire mutexes in an ISR.
- Specify thread priorities, stack reservations and synchronization ownership. Prefer static storage and bounded queues.
- Validate UART argument counts, numeric ranges and state names before changing configuration.
- Report unavailable peripherals and failed operations. Never replace real sensor errors with fabricated temperatures.
- Keep intentional watchdog resets behind explicit commands. A waiting-for-reset message is not reboot evidence.
- Preserve GPIO polarity and board aliases; check pin ownership against devicetree and board schematics.

## Evidence and licensing

Separate source review, compilation, offline checks and physical observations. Hardware evidence should identify the board revision, image/commit, tools, command sequence and actual terminal output. Label synthetic events and simulated readings; never invent screenshots, timing distributions or PASS results.

Derive memory figures from linker/ELF data. Static stack reservation does not establish runtime margin; record watermarks separately. Retain original evidence provenance when adding newer measurements.

Preserve the existing BSD 3-Clause license and all NXP copyright/SPDX notices. Do not relicense third-party board files or vendor external SDK/Zephyr dependency trees into this repository.
