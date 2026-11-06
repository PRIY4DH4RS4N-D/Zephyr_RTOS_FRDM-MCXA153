# 01_rtos_message_pipeline: actual static memory report

Derived from the final ELF load segments, allocated sections, symbol table and linker MEMORY regions. Alignment/gaps count against used space. These are static reservations, not measured runtime usage.

| Region | Used bytes | Capacity bytes | Remaining bytes | Utilization |
|---|---:|---:|---:|---:|
| Flash image including initialized-data load image | 78156 | 131072 | 52916 | 59.63% |
| RAM including stacks/noinit and alignment | 19176 | 24576 | 5400 | 78.03% |

BSS section bytes: 7263. No-init section bytes (including static stacks): 11488. Initialized RAM sections: 424 bytes. No-init is **not** persistent storage across power loss; these apps do not rely on retained RAM.

Heap pool: 0 bytes. Deferred log buffer: 4096 bytes (already included in RAM). External IRQ slots: 80; this is table capacity, not the number of enabled peripheral interrupts.

Vector symbols from ELF:

| Symbol | Address | Symbol bytes |
|---|---|---:|
| `_vector_table` | `0x00000000` | 64 |
| `_irq_vector_table` | `0x00000040` | 320 |
| `_sw_isr_table` | `0x0000e8b8` | 640 |

Vector/ISR tables already contribute to the region totals; do not add them again. Zero symbol sizes mean the assembler/linker did not attach a size, not zero memory usage. See the map for placement and the build audit JSON for reset-vector checks.

Static stack objects from ELF:

| Object | Reserved bytes |
|---|---:|
| `_k_thread_stack_logger_id` | 1280 |
| `_k_thread_stack_producer_id` | 1024 |
| `_k_thread_stack_worker_id` | 1024 |
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

- `elf` (1719740 bytes on disk): `1e47968cb1ca3703626e331feddb4837ada02dc4d1d0ca390a341e0166eb093f`
- `hex` (219985 bytes on disk): `722dd381ba19f83453970236efb92e183621e883a92631074c3167b1d228ed92`
- `bin` (78156 bytes on disk): `4cf0863fef98810caf69ad83e6253eed39765b6962b3bca8497109fbc837917d`

Text section: 58660 bytes; read-only data section: 16736 bytes (already included in flash). Core exception vector size is measured as the address distance to the external IRQ vector table. Inter-section BIN gaps match the configured fill pattern.
