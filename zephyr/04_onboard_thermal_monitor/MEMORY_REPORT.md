# 04_onboard_thermal_monitor: actual static memory report

Derived from the final ELF load segments, allocated sections, symbol table and linker MEMORY regions. Alignment/gaps count against used space. These are static reservations, not measured runtime usage.

| Region | Used bytes | Capacity bytes | Remaining bytes | Utilization |
|---|---:|---:|---:|---:|
| Flash image including initialized-data load image | 101328 | 131072 | 29744 | 77.31% |
| RAM including stacks/noinit and alignment | 20728 | 24576 | 3848 | 84.34% |

BSS section bytes: 7877. No-init section bytes (including static stacks): 12224. Initialized RAM sections: 620 bytes. No-init is **not** persistent storage across power loss; these apps do not rely on retained RAM.

Heap pool: 0 bytes. Deferred log buffer: 4096 bytes (already included in RAM). External IRQ slots: 80; this is table capacity, not the number of enabled peripheral interrupts.

Vector symbols from ELF:

| Symbol | Address | Symbol bytes |
|---|---|---:|
| `_vector_table` | `0x00000000` | 64 |
| `_irq_vector_table` | `0x00000040` | 320 |
| `_sw_isr_table` | `0x00012fec` | 640 |

Vector/ISR tables already contribute to the region totals; do not add them again. Zero symbol sizes mean the assembler/linker did not attach a size, not zero memory usage. See the map for placement and the build audit JSON for reset-vector checks.

Static stack objects from ELF:

| Object | Reserved bytes |
|---|---:|
| `_k_thread_stack_sensor_thread` | 1536 |
| `i3c_ibi_work_q_stack` | 1024 |
| `logging_stack` | 1024 |
| `shell_uart_stack` | 2048 |
| `sys_work_q_stack` | 1536 |
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
CONFIG_I3C_IBI_WORKQUEUE_STACK_SIZE=1024
```

Runtime stack watermarks: **PENDING**. Run `resources` after exercising all commands and operating modes. The linker remaining RAM is unreserved capacity; it is not a running heap or proven stack margin.

Offline audit: PASS. ARM32 ELF; initial SP and Thumb reset vector valid; ELF/HEX/BIN contents match; expected UART/shell/driver configuration present. Physical flashing and runtime behavior: **PENDING**.

Artifact SHA-256:

- `elf` (2596128 bytes on disk): `a917d411b2970b19b7e2e877fd7d4fec00e18939b37174664eba4273fae78c2d`
- `hex` (285124 bytes on disk): `c82a9cce0844ca5f1dda2d04af0a631f9eb788642933277617f8feb6c5d7f4b3`
- `bin` (101328 bytes on disk): `edab522e98fd144dad7ef340ff6b9a229eb9a2c3f7d6634d02eb89b168d2f153`

Text section: 76732 bytes; read-only data section: 21376 bytes (already included in flash). Core exception vector size is measured as the address distance to the external IRQ vector table. Inter-section BIN gaps match the configured fill pattern.
