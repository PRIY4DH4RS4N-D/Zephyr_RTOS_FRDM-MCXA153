# Memory utilization

## Supplied successful build snapshot

These figures are the linker region summary supplied for the completed build. `KB` is treated as 1024 bytes, consistent with the reported percentages. Remaining capacity is region size minus used size.

| Linker region | Purpose | Used | Region size | Remaining | Utilization |
| --- | --- | ---: | ---: | ---: | ---: |
| `m_interrupts` | Vector table region | 384 B | 512 B | 128 B | 75.00% |
| `m_text` | Flash code and read-only content region | 9,576 B | 130,560 B | 120,984 B | 7.33% |
| `m_data` | SRAM region for linked data and reservations | 3,160 B | 24,576 B (24 KiB) | 21,416 B | 12.86% |
| `m_sramx0` | Separate SRAMX0 region | 0 B | 8,192 B (8 KiB) | 8,192 B | 0.00% |

The vector table occupies 384 of its 512 allocated bytes, leaving 128 bytes in that dedicated region. The code region has 120,984 bytes available within its linker allocation. The data region has 21,416 bytes unallocated by the linker; SRAMX0 has no placed sections in this snapshot. These are separate regions, so the free bytes should not be treated as a single interchangeable pool.

The `m_data` figure is a **linker allocation**, not a measurement of live RAM use. It can include initialized data, zero-initialized data, and linker reservations. This report does not establish stack high-water mark, heap usage, or peak runtime RAM consumption. Inspect the map file and measure stack/heap at runtime for those questions. Likewise, `m_text` is the linker region figure, not a standalone count of every physical Flash byte; the vector region is reported separately.

## Repackaged checkout build

A clean configure and build of this repository's `debug` preset produced `frdm_mcxa153_rgb_controller.elf`. Its linker report was `m_interrupts` 384 B, `m_text` 9,532 B, `m_data` 3,160 B, and `m_sramx0` 0 B. The 44-byte `m_text` difference from the supplied snapshot follows the selected three-pin board mux and reduced source set. For that build, `m_text` has 121,028 B remaining (7.30% used); other remaining capacities are unchanged. The repository does not commit the ELF or map file.
