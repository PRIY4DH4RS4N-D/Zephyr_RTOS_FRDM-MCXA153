[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][ValidateSet('01_rtos_message_pipeline','02_interrupt_button_fsm','03_realtime_jitter_profiler','04_onboard_thermal_monitor','05_task_watchdog_supervisor')][string]$Project,
 [string]$Workspace='C:\zephyr-work',
 [string]$Toolchain="$Workspace\arm-gnu-14.2",
 [switch]$Pristine,
 [switch]$InteractiveOnly
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root $Project
$west=Join-Path $Workspace '.venv\Scripts\west.exe'
$env:ZEPHYR_BASE=Join-Path $Workspace 'zephyr'
$env:ZEPHYR_TOOLCHAIN_VARIANT='gnuarmemb'
if($Toolchain -match '\s'){throw 'Use a compiler directory junction without spaces; see BUILD_GUIDE.md'}
$env:GNUARMEMB_TOOLCHAIN_PATH=$Toolchain
$env:PATH="$(Join-Path $Workspace '.venv\Scripts');$env:PATH"
foreach($required in @($west,(Join-Path $Toolchain 'bin\arm-none-eabi-gcc.exe'))){if(-not(Test-Path -LiteralPath $required)){throw "Missing dependency: $required"}}
$buildDir=if($InteractiveOnly){Join-Path $app 'build-interactive'}else{Join-Path $app 'build-verified'}
$pristineMode=if($Pristine){'always'}else{'auto'}
$argsList=@('build','-p',$pristineMode,'-b','frdm_mcxa153','-d',$buildDir,'-o=-j4',$app,'--')
if($InteractiveOnly){$argsList+='-DCONFIG_APP_AUTOTEST=n'}
Push-Location $Workspace
try {
 & (Join-Path $Workspace '.venv\Scripts\python.exe') (Join-Path $PSScriptRoot 'run_build.py') (Join-Path $app 'build.log') @argsList
 if($LASTEXITCODE -ne 0){throw "Build failed: $Project (exit $LASTEXITCODE)"}
 foreach($extension in @('elf','hex','bin')){if(-not(Test-Path (Join-Path $buildDir "zephyr\zephyr.$extension"))){throw "Missing zephyr.$extension"}}
} finally { Pop-Location }
