"""Run ARM GCC's analyzer on application C using real board compile flags."""
import ctypes
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def split_windows(command):
    argc = ctypes.c_int()
    parse = ctypes.windll.shell32.CommandLineToArgvW
    parse.restype = ctypes.POINTER(ctypes.c_wchar_p)
    parse.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
    pointer = parse(command, ctypes.byref(argc))
    result = [pointer[i] for i in range(argc.value)]
    ctypes.windll.kernel32.LocalFree(ctypes.cast(pointer, ctypes.c_void_p))
    return result

lines = ["ARM GNU static analyzer: real Zephyr board compiler flags; -fanalyzer -Werror.",
         "This is offline source analysis, not runtime/hardware testing.", ""]
for app in sorted(ROOT.glob("0*")):
    if not app.is_dir():
        continue
    build = app / "build-verified"
    commands = json.loads((build / "compile_commands.json").read_text())
    output = build / "analysis"
    output.mkdir(exist_ok=True)
    count = 0
    for entry in commands:
        source = Path(entry["file"])
        if source.parent != app / "src":
            continue
        args = entry.get("arguments") or split_windows(entry["command"])
        args[args.index("-o") + 1] = str(output / (source.stem + ".o"))
        args += ["-fanalyzer", "-Werror"]
        result = subprocess.run(args, cwd=entry["directory"], capture_output=True, text=True)
        lines.append(f"{app.name}/{source.name}: {'PASS' if result.returncode==0 else 'FAIL'}")
        if result.stdout or result.stderr:
            lines.append(result.stdout + result.stderr)
        if result.returncode:
            (ROOT / "docs/static_analysis.txt").write_text("\n".join(lines))
            raise SystemExit(result.returncode)
        count += 1
    if count != 2:
        raise RuntimeError(f"Expected main.c and common.c in {app.name}")
(ROOT / "docs/static_analysis.txt").write_text("\n".join(lines)+"\n")
print("\n".join(lines))
