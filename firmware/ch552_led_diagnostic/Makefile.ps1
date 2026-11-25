param(
  [string]$Sdcc = 'E:\SDCC\bin\sdcc.exe',
  [string]$OutDir = (Join-Path $PSScriptRoot 'build'),
  [switch]$GpioProbe
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$stem = 'sl25_led_diagnostic'
$defines = @()
if ($GpioProbe) {
  $stem = 'sl25_gpio_probe'
  $defines = @('-DSL25_GPIO_PROBE=1')
}
& $Sdcc @defines -mmcs51 --model-small --opt-code-size --iram-size 256 --xram-size 1024 --code-size 14336 --out-fmt-ihx -o "$OutDir\$stem.ihx" "$PSScriptRoot\main.c"
if ($LASTEXITCODE -ne 0) { throw 'SDCC build failed' }
& (Join-Path (Split-Path $Sdcc) 'packihx.exe') "$OutDir\$stem.ihx" | Set-Content -Encoding ascii "$OutDir\$stem.hex"
if ($LASTEXITCODE -ne 0) { throw 'packihx failed' }
Write-Host "HEX: $OutDir\$stem.hex"
