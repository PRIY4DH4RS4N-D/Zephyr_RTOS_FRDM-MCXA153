"""Stream merged native diagnostics without PowerShell's stderr ErrorRecords."""
import subprocess
import sys
from datetime import datetime
from pathlib import Path

log_path = Path(sys.argv[1])
with log_path.open("a", encoding="utf-8") as log:
    log.write(f"\nBuild invocation {datetime.now().isoformat(timespec='seconds')}\n")
    log.flush()
    process = subprocess.Popen(
        [sys.executable, "-m", "west", *sys.argv[2:]],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace",
    )
    for line in process.stdout:
        log.write(line)
        log.flush()
        print(line, end="", flush=True)
    sys.exit(process.wait())
