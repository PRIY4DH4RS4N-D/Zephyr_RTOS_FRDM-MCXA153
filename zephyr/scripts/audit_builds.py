"""Read real linker/ELF data; verify board configs, vectors, HEX and BIN.

This is an offline audit. It does not open a serial port or debug probe.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[1]

def require(ok, message):
    if not ok:
        raise RuntimeError(message)

def inspect(app, build_name):
    z = app / build_name / "zephyr"
    for name in ("zephyr.elf", "zephyr.hex", "zephyr.bin", "zephyr.map", ".config", "zephyr.dts"):
        require((z / name).is_file(), f"{app.name}: missing {name}")
    cfg = dict(re.findall(r"^(CONFIG_\w+)=(.+)$", (z / ".config").read_text(), re.M))
    for key in ("CONFIG_BOARD_FRDM_MCXA153", "CONFIG_SOC_MCXA153", "CONFIG_SHELL", "CONFIG_UART_CONSOLE", "CONFIG_SHELL_BACKEND_SERIAL", "CONFIG_STACK_SENTINEL", "CONFIG_INIT_STACKS"):
        require(cfg.get(key) == "y", f"{app.name}: invalid {key}")
    require(cfg.get("CONFIG_HEAP_MEM_POOL_SIZE") == "0", "Unexpected heap")
    require(cfg.get("CONFIG_BOOTLOADER_MCUBOOT") != "y", "Unexpected MCUboot image")
    dts = (z / "zephyr.dts").read_text()
    require("current-speed = < 0x1c200 >" in dts or re.search(r"current-speed\s*=\s*<\s*(115200|0x1c200)\s*>", dts), "115200 UART not found")
    mapping = (z / "zephyr.map").read_text(errors="replace")
    memory = mapping.split("Memory Configuration", 1)[1].split("Linker script and memory map", 1)[0]
    regions = {n: (int(o, 16), int(s, 16)) for n, o, s in re.findall(r"^([\w]+)\s+(0x[\da-fA-F]+)\s+(0x[\da-fA-F]+)", memory, re.M)}
    flash_origin, flash_size = regions["FLASH"]
    ram_origin, ram_size = regions["RAM"]
    binary = (z / "zephyr.bin").read_bytes()
    with (z / "zephyr.elf").open("rb") as handle:
        elf = ELFFile(handle)
        require(elf.header["e_machine"] == "EM_ARM" and elf.elfclass == 32, "ELF is not ARM32")
        sections = list(elf.iter_sections())
        table = elf.get_section_by_name(".symtab")
        symbols = {s.name: (s["st_value"], s["st_size"]) for s in table.iter_symbols() if s.name}
        load_ends = []
        covered = bytearray(len(binary))
        for segment in elf.iter_segments():
            if segment["p_type"] == "PT_LOAD" and flash_origin <= segment["p_paddr"] < flash_origin + flash_size and segment["p_filesz"]:
                offset = segment["p_paddr"] - flash_origin
                load_ends.append(offset + segment["p_filesz"])
                # Objcopy uses configured fill bytes in inter-section gaps;
                # ELF segment padding is not allocated section content.
                for section in sections:
                    if section["sh_flags"] & 2 and section["sh_type"] != "SHT_NOBITS" and segment.section_in_segment(section):
                        data = section.data()
                        start = offset + section["sh_addr"] - segment["p_vaddr"]
                        require(binary[start:start+len(data)] == data, f"{app.name}: ELF section {section.name} differs from BIN")
                        covered[start:start+len(data)] = bytes([1]) * len(data)
        fill = int(cfg["CONFIG_BUILD_GAP_FILL_PATTERN"], 0)
        require(all(used or value == fill for used, value in zip(covered, binary)), "Unexpected BIN gap-fill bytes")
        require(load_ends, "No flash load segments")
        flash_used = max(load_ends)
        ram_sections = [s for s in sections if s["sh_flags"] & 2 and ram_origin <= s["sh_addr"] < ram_origin + ram_size and s["sh_size"]]
        ram_used = max(s["sh_addr"] + s["sh_size"] for s in ram_sections) - ram_origin
        bss = sum(s["sh_size"] for s in ram_sections if "bss" in s.name.lower())
        noinit = sum(s["sh_size"] for s in ram_sections if "noinit" in s.name.lower())
        initialized = sum(s["sh_size"] for s in ram_sections if s["sh_type"] != "SHT_NOBITS")
        vector_symbols = {n: {"address": v, "bytes": s} for n, (v, s) in symbols.items() if n in ("_vector_table", "_irq_vector_table", "_sw_isr_table")}
        if "_vector_table" in vector_symbols and "_irq_vector_table" in vector_symbols:
            vector_symbols["_vector_table"]["bytes"] = vector_symbols["_irq_vector_table"]["address"] - vector_symbols["_vector_table"]["address"]
        text_bytes = sum(s["sh_size"] for s in sections if s.name in ("text", ".text"))
        rodata_bytes = sum(s["sh_size"] for s in sections if s.name in ("rodata", ".rodata"))
        stacks = {n: s for n, (v, s) in symbols.items() if s and ("stack" in n.lower()) and ram_origin <= v < ram_origin + ram_size}
        used_symbols = set(symbols)
    require(flash_used <= flash_size and ram_used <= ram_size, "Region overflow")
    require(len(binary) == flash_used, "Unexpected binary padding/length")
    sp, reset = struct.unpack_from("<II", binary)
    require(ram_origin <= sp <= ram_origin + ram_size and sp % 8 == 0, "Invalid initial stack pointer")
    require(reset & 1 and flash_origin <= (reset & ~1) < flash_origin + flash_used, "Invalid Thumb reset vector")
    upper = 0
    hex_bytes = 0
    eof = False
    for line in (z / "zephyr.hex").read_text().splitlines():
        record = bytes.fromhex(line.removeprefix(":"))
        require(sum(record) % 256 == 0, "HEX checksum failure")
        count, addr, kind = record[0], int.from_bytes(record[1:3], "big"), record[3]
        require(len(record) == count + 5, "Malformed HEX record")
        data = record[4:4+count]
        if kind == 4:
            upper = int.from_bytes(data, "big") << 16
        elif kind == 2:
            upper = int.from_bytes(data, "big") << 4
        elif kind == 0:
            offset = upper + addr - flash_origin
            require(offset >= 0 and binary[offset:offset+count] == data, "HEX bytes differ from BIN")
            hex_bytes += count
        elif kind == 1:
            eof = True
    require(eof and hex_bytes > 0, "Incomplete HEX")
    if app.name.startswith("04_"):
        for key in ("CONFIG_SENSOR", "CONFIG_P3T1755", "CONFIG_I3C", "CONFIG_I3C_MCUX"):
            require(cfg.get(key) == "y", f"Missing sensor feature {key}")
        require("p3t1755_sample_fetch" in used_symbols, "Sensor implementation absent from ELF")
    if app.name.startswith("05_"):
        for key in ("CONFIG_TASK_WDT", "CONFIG_TASK_WDT_HW_FALLBACK", "CONFIG_WDT_MCUX_WWDT", "CONFIG_HWINFO_MCUX_MCX_CMC"):
            require(cfg.get(key) == "y", f"Missing watchdog feature {key}")
        require("task_wdt_feed" in used_symbols and "mcux_wwdt_feed" in used_symbols, "Watchdog implementation absent from ELF")
    result = dict(project=app.name, build=build_name, status="PASS (offline build/configuration/binary audit)", flash_bytes=flash_used, flash_region_bytes=flash_size, flash_free_bytes=flash_size-flash_used, flash_percent=round(100*flash_used/flash_size,2), ram_bytes=ram_used, ram_region_bytes=ram_size, ram_free_bytes=ram_size-ram_used, ram_percent=round(100*ram_used/ram_size,2), bss_bytes=bss, noinit_bytes=noinit, initialized_ram_section_bytes=initialized, vectors=vector_symbols, configured_external_irqs=int(cfg["CONFIG_NUM_IRQS"]), stack_symbols=stacks, stack_config={k:v for k,v in cfg.items() if "STACK_SIZE" in k}, heap_pool_bytes=int(cfg["CONFIG_HEAP_MEM_POOL_SIZE"]), log_buffer_bytes=int(cfg["CONFIG_LOG_BUFFER_SIZE"]), initial_sp=hex(sp), reset_vector=hex(reset), artifacts={ext: {"bytes":(z/f"zephyr.{ext}").stat().st_size,"sha256":hashlib.sha256((z/f"zephyr.{ext}").read_bytes()).hexdigest()} for ext in ("elf","hex","bin")}, hardware_verification="PENDING")
    result.update(text_section_bytes=text_bytes, rodata_section_bytes=rodata_bytes)
    (app / "memory_report.json").write_text(json.dumps(result, indent=2)+"\n")
    lines = [f"# {app.name}: actual static memory report", "", "Derived from the final ELF load segments, allocated sections, symbol table and linker MEMORY regions. Alignment/gaps count against used space. These are static reservations, not measured runtime usage.", "", "| Region | Used bytes | Capacity bytes | Remaining bytes | Utilization |", "|---|---:|---:|---:|---:|", f"| Flash image including initialized-data load image | {flash_used} | {flash_size} | {flash_size-flash_used} | {result['flash_percent']}% |", f"| RAM including stacks/noinit and alignment | {ram_used} | {ram_size} | {ram_size-ram_used} | {result['ram_percent']}% |", "", f"BSS section bytes: {bss}. No-init section bytes (including static stacks): {noinit}. Initialized RAM sections: {initialized} bytes. No-init is **not** persistent storage across power loss; these apps do not rely on retained RAM.", "", f"Heap pool: {result['heap_pool_bytes']} bytes. Deferred log buffer: {result['log_buffer_bytes']} bytes (already included in RAM). External IRQ slots: {result['configured_external_irqs']}; this is table capacity, not the number of enabled peripheral interrupts.", "", "Vector symbols from ELF:", "", "| Symbol | Address | Symbol bytes |", "|---|---|---:|"]
    lines += [f"| `{n}` | `{v['address']:#010x}` | {v['bytes']} |" for n,v in vector_symbols.items()]
    lines += ["", "Vector/ISR tables already contribute to the region totals; do not add them again. Zero symbol sizes mean the assembler/linker did not attach a size, not zero memory usage. See the map for placement and the build audit JSON for reset-vector checks.", "", "Static stack objects from ELF:", "", "| Object | Reserved bytes |", "|---|---:|"]
    lines += [f"| `{n}` | {s} |" for n,s in sorted(stacks.items())]
    lines += ["", "Stack-related final Kconfig:", "", "```text"] + [f"{k}={v}" for k,v in result["stack_config"].items()] + ["```", "", "Runtime stack watermarks: **PENDING**. Run `resources` after exercising all commands and operating modes. The linker remaining RAM is unreserved capacity; it is not a running heap or proven stack margin.", "", "Offline audit: PASS. ARM32 ELF; initial SP and Thumb reset vector valid; ELF/HEX/BIN contents match; expected UART/shell/driver configuration present. Physical flashing and runtime behavior: **PENDING**.", "", "Artifact SHA-256:", ""]
    lines += [f"- `{ext}` ({v['bytes']} bytes on disk): `{v['sha256']}`" for ext,v in result["artifacts"].items()]
    lines += ["", f"Text section: {text_bytes} bytes; read-only data section: {rodata_bytes} bytes (already included in flash). Core exception vector size is measured as the address distance to the external IRQ vector table. Inter-section BIN gaps match the configured fill pattern."]
    (app / "MEMORY_REPORT.md").write_text("\n".join(lines)+"\n")
    return result

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build-verified")
    args = parser.parse_args()
    results = [inspect(p, args.build_dir) for p in sorted(ROOT.glob("0*")) if p.is_dir()]
    require(len(results)==5, "Expected exactly five applications")
    (ROOT/"docs/build_audit.json").write_text(json.dumps(results,indent=2)+"\n")
    lines=["# Memory comparison", "", "Actual static linker/ELF usage. Physical and runtime verification remain PENDING.", "", "| Project | Flash bytes | Flash % | RAM bytes | RAM % | BSS bytes | Flash remaining | RAM remaining |", "|---|---:|---:|---:|---:|---:|---:|---:|"]
    for r in results:
        lines.append(f"| [{r['project']}](../{r['project']}/MEMORY_REPORT.md) | {r['flash_bytes']} | {r['flash_percent']} | {r['ram_bytes']} | {r['ram_percent']} | {r['bss_bytes']} | {r['flash_free_bytes']} | {r['ram_free_bytes']} |")
        print(f"PASS {r['project']}: flash={r['flash_bytes']}/{r['flash_region_bytes']} RAM={r['ram_bytes']}/{r['ram_region_bytes']}")
    (ROOT/"docs/memory_comparison.md").write_text("\n".join(lines)+"\n")

if __name__ == "__main__":
    main()
