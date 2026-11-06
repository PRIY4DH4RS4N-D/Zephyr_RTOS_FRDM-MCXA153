# 03_realtime_jitter_profiler: actual static memory report

Derived from the final ELF load segments, allocated sections, symbol table and linker MEMORY regions. Alignment/gaps count against used space. These are static reservations, not measured runtime usage.

| Region | Used bytes | Capacity bytes | Remaining bytes | Utilization |
|---|---:|---:|---:|---:|
| Flash image including initialized-data load image | 76724 | 131072 | 54348 | 58.54% |
| RAM including stacks/noinit and alignment | 17136 | 24576 | 7440 | 69.73% |

BSS section bytes: 7116. No-init section bytes (including static stacks): 9728. Initialized RAM sections: 288 bytes. No-init is **not** persistent storage across power loss; these apps do not rely on retained RAM.

Heap pool: 0 bytes. Deferred log buffer: 4096 bytes (already included in RAM). External IRQ slots: 80; this is table capacity, not the number of enabled peripheral interrupts.

Vector symbols from ELF:

| Symbol | Address | Symbol bytes |
|---|---|---:|
| `_vector_table` | `0x00000000` | 64 |
| `_irq_vector_table` | `0x00000040` | 320 |
| `_sw_isr_table` | `0x0000e458` | 640 |

Vector/ISR tables already contribute to the region totals; do not add them again. Zero symbol sizes mean the assembler/linker did not attach a size, not zero memory usage. See the map for placement and the build audit JSON for reset-vector checks.

Static stack objects from ELF:

| Object | Reserved bytes |
|---|---:|
| `_k_thread_stack_load_id` | 768 |
| `_k_thread_stack_sampler_id` | 1024 |
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

- `elf` (1663768 bytes on disk): `418d92e213ef0a14c71d38c80ca341ec89052cf6a824daa521773a4db23e6a4f`
- `hex` (215951 bytes on disk): `5777c24a50db739bb07d560553ab429ff6567c60627360a557d2d29983bdd7cb`
- `bin` (76724 bytes on disk): `6a1a1c7b8120096ddc9a2fae3454ab39673f8b061672ab650dc15f4a12c0346c`

Text section: 57540 bytes; read-only data section: 16608 bytes (already included in flash). Core exception vector size is measured as the address distance to the external IRQ vector table. Inter-section BIN gaps match the configured fill pattern.
