param(
  [string]$Sdcc = "E:\SDCC\bin\sdcc.exe",
  [string]$OutDir = ".\build"
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
& $Sdcc -mmcs51 --model-small --opt-code-size `
  --iram-size 256 --xram-size 1024 --code-size 14336 `
  --out-fmt-ihx `
  -o "$OutDir\sl25_codex_status.ihx" ".\main.c"
if ($LASTEXITCODE -ne 0) { throw "SDCC 编译失败" }
& (Join-Path (Split-Path $Sdcc) "packihx.exe") "$OutDir\sl25_codex_status.ihx" |
  Set-Content -Encoding ascii "$OutDir\sl25_codex_status.hex"
if ($LASTEXITCODE -ne 0) { throw "packihx 转换失败" }
Write-Host "生成: $OutDir\sl25_codex_status.hex"
