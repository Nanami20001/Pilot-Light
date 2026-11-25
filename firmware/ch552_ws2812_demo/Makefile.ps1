param(
  [string]$Sdcc = 'E:\SDCC\bin\sdcc.exe',
  [string]$OutDir = (Join-Path $PSScriptRoot 'build-white')
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
& $Sdcc -mmcs51 --model-small --opt-code-size --iram-size 256 --xram-size 1024 --code-size 14336 --out-fmt-ihx -o "$OutDir\sl25_ws2812_white.ihx" "$PSScriptRoot\main.c"
if ($LASTEXITCODE -ne 0) { throw 'SDCC build failed' }
& (Join-Path (Split-Path $Sdcc) 'packihx.exe') "$OutDir\sl25_ws2812_white.ihx" | Set-Content -Encoding ascii "$OutDir\sl25_ws2812_white.hex"
if ($LASTEXITCODE -ne 0) { throw 'packihx failed' }
Write-Host "HEX: $OutDir\sl25_ws2812_white.hex"
