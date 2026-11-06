# Verification boundaries and physical test plan

Four levels are reported separately: source review, compilation/linking, binary/configuration inspection, and physical hardware verification. The build audit validates the first three using generated files. All hardware observations, startup self-test execution, real temperature accuracy, debounce behavior, jitter values, stack watermarks and watchdog reset/recovery remain **PENDING** until flashed by the user. An ELF is not evidence that a board test passed.

Every standard build contains predefined startup tests controlled by `CONFIG_APP_AUTOTEST=y`. The pipeline fills a gated queue, tests overflow, and checks a 123-to-246 transaction; FSM tests transition rules plus a synthetic deferred event; profiler tests statistics arithmetic and periodic progress; thermal tests classification boundary values without impersonating sensor data; watchdog checks healthy heartbeats and absence of automatic faults. Runtime results use `[PASS]`/`[FAIL]` tags. Synthetic events and simulated values are labelled separately from physical inputs.

| Application | Manual demonstration | Acceptance evidence to capture |
|---|---|---|
| Pipeline | `pipeline inject 123`, `pipeline start`, `pipeline status`, `pipeline stop` | Matching ID/result 246; accepted/processed/logged advance; queues drain after stop |
| Button FSM | Press SW2 repeatedly; SW3 in ALERT; `fsm mode CONFIGURATION` | One accepted event per deliberate press; correct LED/state; raw/debounce counters respond to actual bounce if present |
| Jitter | `bench start`, wait, `bench report`, `bench load 20000`, report, `bench stop` | Measured sample count, signed extrema/mean and absolute mean; high-priority sampler progresses under load; no invented timing targets |
| Thermal | `thermal read`, `thermal status`, `thermal threshold 35 45` | Actual source SENSOR with successful device/read, or explicit read failure without a temperature value |
| Thermal test | `thermal simulate 25`, then 35, then 45, then `thermal simulate off` | Explicit SIMULATED labels and NORMAL/WARNING/CRITICAL; filter resets on source changes |
| Watchdog recover | `guard fault 0 recover`, wait >2 s, `guard status` | Timeout recorded, recoveries advance, heartbeat resumes without reset |
| Watchdog reset | **Only when deliberately ready:** `guard fault 1 reset` | Fault latch log, actual reboot banner after timeout, CMC `RESET_WATCHDOG` cause reported on the next boot |

Watchdog reset tests are never part of startup tests or build scripts. If the watchdog reboot is not physically observed and corroborated by reset-cause diagnostics, record the test as failed/pending rather than interpreting the waiting message as success. Normal software recovery clears the injected fault and resumes the worker; it does not claim to repair arbitrary application corruption.

Run `help` and `resources` in each app. `resources` reports each thread's reserved stack and observed unused watermark. Exercise all commands and maximum workload before judging margin. Static stack reservation is included in linker RAM usage; watermark observations are runtime measurements. Static compile checks cannot prove worst-case execution time, stack sufficiency or bounded electrical interrupt latency.

Invalid command cases: missing arguments; extra arguments; `pipeline inject 999999999999999`; `bench period 0`; period changes while running; `fsm mode INVALID`; warning >= critical; invalid fault index. Expect a shell argument error or negative errno, with no accepted configuration change. Capture output rather than assuming a particular shell error wording.

The profiler measures **inter-release jitter**, not an external GPIO waveform or end-to-end ISR latency. Missed periods count whole releases skipped when the sampler wakes at least one full period late. It does not count every sub-period positive jitter as a miss. Measurements include scheduler/timer overhead and can be perturbed by logging, command processing and debugging. The low-priority workload is preemptible. A run stores up to one million statistics samples; no heap grows with runtime.

Logging uses a bounded deferred buffer and the shell backend. Excessive log volume can drop log messages; queue/heartbeat counters remain the primary state evidence. Connect and log UART before fault tests whenever possible.
