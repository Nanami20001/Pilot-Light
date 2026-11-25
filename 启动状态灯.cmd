@echo off
setlocal
set "SL25_AGENT=%~dp0host\codex_status_agent.mjs"
powershell.exe -NoProfile -Command ^
  "$ErrorActionPreference = 'Stop';" ^
  "$mutex = [Threading.Mutex]::new($false, 'Local\SL25_Status_Lamp_Launch');" ^
  "$held = $false; try {" ^
  "  try { $held = $mutex.WaitOne(10000) } catch [Threading.AbandonedMutexException] { $held = $true }" ^
  "  if (-not $held) { throw 'Another launcher is still starting the lamp.' }" ^
  "  $existing = Get-CimInstance Win32_Process -Filter 'Name = ''node.exe''' | Where-Object { $_.CommandLine -match 'codex_status_agent\.mjs(?:\x22|\s)' -and $_.CommandLine -match '--vid\s+0xF055\b' -and $_.CommandLine -match '--pid\s+0x2525\b' -and $_.CommandLine -notmatch '--(?:once|dry-run|state-file)\b' };" ^
  "  if ($existing) { Write-Host 'Status lamp is already running.'; exit 0 }" ^
  "  $script = (Resolve-Path -LiteralPath $env:SL25_AGENT).Path;" ^
  "  $arguments = [char]34 + $script + [char]34 + ' --vid 0xF055 --pid 0x2525 --verbose';" ^
  "  $agent = Start-Process -FilePath (Get-Command node.exe).Source -ArgumentList $arguments -WorkingDirectory (Split-Path -Parent $script) -WindowStyle Hidden -PassThru;" ^
  "  if ($agent.WaitForExit(2500)) { throw 'Lamp agent exited. Check the USB connection and Node.js installation.' }" ^
  "  Write-Host 'Status lamp started.';" ^
  "} finally { if ($held) { $mutex.ReleaseMutex() }; $mutex.Dispose() }"
if errorlevel 1 (
  if /i not "%~1"=="--autostart" pause
  exit /b 1
)
