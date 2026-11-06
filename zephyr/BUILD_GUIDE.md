# Building the Zephyr portfolio

The five applications build independently for `frdm_mcxa153`, resolving to `frdm_mcxa153/mcxa153`. Keep the external Zephyr west workspace separate from both this repository and the MCUXpresso SDK workspace. Follow [installation instructions](docs/zephyr_installation.md) first.

## Repository and original workspace

The publication checkout is `C:\MCU_Expresso_Projects\Zephyr_RTOS_FRDM-MCXA153`; its application collection is the `zephyr` subdirectory. The supplied original workspace is `C:\MCU_Expresso_Projects\FRDM-MCXA153_Zephyr_RTOS` and remains unchanged. Historical memory/artifact reports describe builds in that original workspace. Scripts locate applications relative to themselves, so both layouts work.

From the repository root:

```powershell
Set-Location .\zephyr
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_all.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_project.ps1 -Project 03_realtime_jitter_profiler -Pristine
```

The default external workspace is `C:\zephyr-work`; pass `-Workspace` to override it. The individual build script also accepts `-Toolchain`. Its default is `<workspace>\arm-gnu-14.2`, a space-free junction to the existing ARM GNU installation. The compiler is selected explicitly through `gnuarmemb`, avoiding unrelated compilers on system PATH. No registry or machine PATH changes are required.

`-Pristine` requests a fresh west build. Omit it for incremental builds. `-InteractiveOnly` disables predefined startup tests and uses a separate `build-interactive` directory; normal builds use `build-verified`. Runtime results remain pending until physically executed, regardless of build mode.

## Direct west command

From this repository's `zephyr` directory:

```powershell
$workspace='C:\zephyr-work'
$app=(Resolve-Path .\01_rtos_message_pipeline).Path
$env:ZEPHYR_BASE="$workspace\zephyr"
$env:ZEPHYR_TOOLCHAIN_VARIANT='gnuarmemb'
$env:GNUARMEMB_TOOLCHAIN_PATH="$workspace\arm-gnu-14.2"
$env:PATH="$workspace\.venv\Scripts;$env:PATH"
Push-Location $workspace
try {
    west boards | Select-String frdm_mcxa153
    west build -p always -b frdm_mcxa153 -d "$app\build-verified" $app -o=-j4
} finally { Pop-Location }
```

Select another application by changing only `$app`. Images boot directly from flash address zero; MCUboot is disabled. UART0 is explicitly 115200 baud and USB is disabled in the overlays. The board's fixed partitions do not constrain these standalone images to the MCUboot slot.

## Outputs and audits

Each local build produces:

```text
<application>/build-verified/
  compile_commands.json
  zephyr/zephyr.elf
  zephyr/zephyr.hex
  zephyr/zephyr.bin
  zephyr/zephyr.map
  zephyr/.config
  zephyr/zephyr.dts
```

Generated outputs and local `build.log` files are ignored by Git. Compact memory, source and artifact-fingerprint reports are retained as reproducibility evidence.

After building all five, run from this directory:

```powershell
& C:\zephyr-work\.venv\Scripts\python.exe .\scripts\audit_builds.py
& C:\zephyr-work\.venv\Scripts\python.exe .\scripts\analyze_sources.py
python .\scripts\verify_publication.py
```

`build_all.ps1` already performs the artifact audit. It checks ARM32 ELF/reset vectors, allocated load-section and HEX/BIN agreement, memory regions, UART settings and expected sensor/watchdog drivers. Analysis uses actual application compile flags with `-fanalyzer -Werror`. Audit scripts regenerate local reports; review changes before committing. Original ELF hashes can differ from a new build because of source/debug paths and tool versions.

Assertions and stack sentinels are intentionally enabled. The known assertion-enabled notice and GNU ld RWX load-segment warning are documented in the [verification record](docs/build_verification.md). A standalone DTC executable was absent in the recorded environment; Zephyr's Python EDT parser generated and validated the devicetree successfully.

These scripts never access a probe or serial port. For later deliberate flashing and PuTTY configuration, use [flashing_guide.md](docs/flashing_guide.md).
