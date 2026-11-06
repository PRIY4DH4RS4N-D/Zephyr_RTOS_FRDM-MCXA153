# Environment audit and isolated setup

Audit date: 2026-11-06, Asia/Calcutta.

Existing: west 1.5.0, Python 3.11.9, CMake 4.3.2, Ninja 1.13.2, NXP ARM GNU 14.2.1, LinkServer 25.6.131, VS Code and MCUXpresso SDK 26.09. Zephyr SDK 1.0.1 is installed at `C:\Users\Priyadarsan D\zephyr-sdk-1.0.1`; it was not selected for these builds, which use the existing ARM GNU toolchain through Zephyr's supported `gnuarmemb` integration. The SDK is left unchanged.

The SDK directory active in the original working directory was an MCUXpresso west workspace, not a Zephyr workspace. It was not modified. No registered Zephyr source installation was found in the inspected locations. A separate Zephyr workspace and Python virtual environment were created in `C:\zephyr-work`.

Pinned upstream sources:

| Repository | Revision |
|---|---|
| zephyr | v4.3.0, `3568e1b6d5cdd51a6b964a2a1d6d29200fea2056` |
| cmsis | `512cc7e895e8491696b61f7ba8066b4a182569b8` |
| cmsis_6 | `30a859f44ef8ab4dc8f84b03ed586fd16ccf9d74` |
| hal_nxp | `6d6ab91d5cc71c872353af1957d19f3d930df0fd` |

Only the HAL modules needed by these applications were fetched. HAL_NXP is the version pinned by Zephyr, separate from the standalone MCUXpresso SDK. Firmware uses Zephyr's public APIs; Zephyr drivers internally use their pinned HAL.

Equivalent setup on a new Windows host:

```powershell
git clone --depth 1 --branch v4.3.0 https://github.com/zephyrproject-rtos/zephyr.git C:\zephyr-work\zephyr
Set-Location C:\zephyr-work
python -m venv .venv
& .\.venv\Scripts\python.exe -m pip install -r .\zephyr\scripts\requirements-base.txt
& .\.venv\Scripts\python.exe -m pip install cmake==3.31.6
New-Item -ItemType Junction -Path C:\zephyr-work\arm-gnu-14.2 -Target "$env:USERPROFILE\.mcuxpressotools\arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi"
& .\.venv\Scripts\west.exe init -l zephyr
& .\.venv\Scripts\west.exe update cmsis cmsis_6 hal_nxp --fetch-opt=--depth=1
& .\.venv\Scripts\west.exe boards | Select-String frdm_mcxa153
```

Use the root build scripts to configure the toolchain per process. Nothing is installed into the system Python by this setup. Python dependencies are captured in `python-requirements-lock.txt`. Windows long-path support and a short workspace path are advisable for Zephyr builds. The scripts use `C:\zephyr-work\arm-gnu-14.2`, a directory junction to the existing compiler, because the user profile name contains a space. CMake expands DOS short names back to long names, so a junction is used instead.

Configure failures were encountered while C: ran out of free space. CMake 3.31.6 was installed only into the workspace virtual environment during diagnosis and is pinned for reproducibility; the system CMake remains unchanged. No new compiler installation was necessary. The missing Zephyr sources and Python dependencies were added in the isolated workspace. A source archive download used while checking network access is not a build dependency.
