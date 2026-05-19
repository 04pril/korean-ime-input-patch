$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildDir = Join-Path $root "build"
$releaseDir = Join-Path $root "release"

New-Item -ItemType Directory -Force -Path $buildDir, $releaseDir | Out-Null

$vsDevCmd = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat not found: $vsDevCmd"
}

$src = Join-Path $root "src\d3dx9_27_ime_only.cpp"
$def = Join-Path $root "src\d3dx9_27.def"
$out = Join-Path $buildDir "d3dx9_27.dll"

Push-Location $root
try {
    $batch = Join-Path $buildDir "build_ime_only.cmd"
    $batchContent = @"
call "$vsDevCmd" -arch=x86 -host_arch=x64 >nul
cl /nologo /LD /EHsc /O2 /DWIN32 /D_WINDOWS "$src" /Fe:"$out" /link /nologo /MACHINE:X86 /DEF:"$def" user32.lib imm32.lib
"@
    Set-Content -LiteralPath $batch -Value $batchContent -Encoding ASCII
    cmd /c "`"$batch`""
    Copy-Item -LiteralPath $out -Destination (Join-Path $releaseDir "d3dx9_27.dll") -Force
    Get-Item (Join-Path $releaseDir "d3dx9_27.dll")
}
finally {
    Pop-Location
}
