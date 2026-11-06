# 05_task_watchdog_supervisor: actual static memory report

Derived from the final ELF load segments, allocated sections, symbol table and linker MEMORY regions. Alignment/gaps count against used space. These are static reservations, not measured runtime usage.

| Region | Used bytes | Capacity bytes | Remaining bytes | Utilization |
|---|---:|---:|---:|---:|
| Flash image including initialized-data load image | 81608 | 131072 | 49464 | 62.26% |
| RAM including stacks/noinit and alignment | 19320 | 24576 | 5256 | 78.61% |

BSS section bytes: 7453. No-init section bytes (including static stacks): 11520. Initialized RAM sections: 344 bytes. No-init is **not** persistent storage across power loss; these apps do not rely on retained RAM.

Heap pool: 0 bytes. Deferred log buffer: 4096 bytes (already included in RAM). External IRQ slots: 80; this is table capacity, not the number of enabled peripheral interrupts.

Vector symbols from ELF:

| Symbol | Address | Symbol bytes |
|---|---|---:|
| `_vector_table` | `0x00000000` | 64 |
| `_irq_vector_table` | `0x00000040` | 320 |
| `_sw_isr_table` | `0x0000f184` | 640 |

Vector/ISR tables already contribute to the region totals; do not add them again. Zero symbol sizes mean the assembler/linker did not attach a size, not zero memory usage. See the map for placement and the build audit JSON for reset-vector checks.

Static stack objects from ELF:

| Object | Reserved bytes |
|---|---:|
| `_k_thread_stack_supervisor_id` | 1536 |
| `_k_thread_stack_worker0_id` | 1024 |
| `_k_thread_stack_worker1_id` | 1024 |
| `logging_stack` | 1024 |
| `shell_uart_stack` | 2048 |
| `z_idle_stacks` | 256 |
| `z_interrupt_stacks` | 2048 |
| `z_main_stack` | 2048 |

Stack-related final Kconfig:

```text
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=1536
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_SHELL_STACK_SIZE=2048
CONFIG_IDLE_STACK_SIZE=256
CONFIG_ISR_STACK_SIZE=2048
CONFIG_LOG_PROCESS_THREAD_STACK_SIZE=1024
```

Runtime stack watermarks: **PENDING**. Run `resources` after exercising all commands and operating modes. The linker remaining RAM is unreserved capacity; it is not a running heap or proven stack margin.

Offline audit: PASS. ARM32 ELF; initial SP and Thumb reset vector valid; ELF/HEX/BIN contents match; expected UART/shell/driver configuration present. Physical flashing and runtime behavior: **PENDING**.

Artifact SHA-256:

- `elf` (1790400 bytes on disk): `89c5ec3ea7ff7a4e6166e3920d424d8b8cd4386729989384f717d935e851bc71`
- `hex` (229668 bytes on disk): `f2ca218a296ac1744705715aef619ef9ced393cfcabaed483322b7aed9d914fb`
- `bin` (81608 bytes on disk): `969b73833ee1f6c3093620461499dbc535b442441c7689fb97a4f7c2fd55f5f7`

Text section: 60868 bytes; read-only data section: 17948 bytes (already included in flash). Core exception vector size is measured as the address distance to the external IRQ vector table. Inter-section BIN gaps match the configured fill pattern.
