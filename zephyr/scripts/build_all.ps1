[CmdletBinding()]
param([string]$Workspace='C:\zephyr-work',[switch]$Pristine,[switch]$InteractiveOnly)
$ErrorActionPreference='Stop'
$projects=@('01_rtos_message_pipeline','02_interrupt_button_fsm','03_realtime_jitter_profiler','04_onboard_thermal_monitor','05_task_watchdog_supervisor')
foreach($project in $projects){
 & (Join-Path $PSScriptRoot 'build_project.ps1') -Project $project -Workspace $Workspace -Pristine:$Pristine -InteractiveOnly:$InteractiveOnly
}
$auditDir=if($InteractiveOnly){'build-interactive'}else{'build-verified'}
& (Join-Path $Workspace '.venv\Scripts\python.exe') (Join-Path $PSScriptRoot 'audit_builds.py') --build-dir $auditDir
if($LASTEXITCODE -ne 0){throw 'Binary audit failed'}
