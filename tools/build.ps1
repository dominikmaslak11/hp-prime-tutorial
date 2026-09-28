<#
.SYNOPSIS
  Buduje wszystkie narzędzia HP PPL: ppl.exe, testy, gramatykę VS Code, pliki Notepad++ i paczkę .vsix.

.PARAMETER SkipVsix
  Nie buduj rozszerzenia VS Code (nie wymaga Node.js).

.PARAMETER VsCodeTests
  Uruchom też testy integracyjne rozszerzenia w VS Code (izolowany profil).

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\build.ps1
#>
param(
    [switch]$SkipVsix,
    [switch]$VsCodeTests
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

# MSYS2 UCRT64 (g++, cmake, ninja, node) jeśli jest zainstalowane
$msys = 'C:\msys64\ucrt64\bin'
if (Test-Path $msys) { $env:PATH = "$msys;$env:PATH" }

function Step($name) { Write-Host "`n=== $name ===" -ForegroundColor Cyan }
function Run($exe, [string[]]$arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "$exe $($arguments -join ' ') zakończył się kodem $LASTEXITCODE" }
}

Step 'Kompilacja (CMake + Ninja)'
Run cmake @('-S', '.', '-B', 'build', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release')
Run cmake @('--build', 'build')

Step 'Testy (jednostkowe + wszystkie programy z kursu)'
Run '.\build\ppltests.exe' @()

Step 'Generowanie gramatyki VS Code i plików Notepad++'
Run '.\build\ppl.exe' @('gen', 'tmgrammar', 'vscode\syntaxes\hpppl.tmLanguage.json')
Run '.\build\ppl.exe' @('gen', 'notepadpp', 'notepadpp')

if (-not $SkipVsix) {
    Step 'Rozszerzenie VS Code (.vsix)'
    New-Item -ItemType Directory -Force -Path 'vscode\bin', 'vscode\data', 'dist' | Out-Null
    Copy-Item 'build\ppl.exe' 'vscode\bin\ppl.exe' -Force
    Copy-Item 'data\commands.json' 'vscode\data\commands.json' -Force
    Push-Location vscode
    try {
        if (-not (Test-Path 'node_modules')) { Run npm @('install', '--no-audit', '--no-fund') }
        Run node @('node_modules\@vscode\vsce\vsce', 'package', '--skip-license', '--out', '..\dist\')
        if ($VsCodeTests) {
            Step 'Testy integracyjne w VS Code'
            Run node @('test\run.js')
        }
    } finally { Pop-Location }
}

Step 'Gotowe'
Write-Host "ppl.exe:        $root\build\ppl.exe"
if (-not $SkipVsix) { Get-ChildItem "$root\dist\*.vsix" | ForEach-Object { Write-Host "Rozszerzenie:   $($_.FullName)" } }
Write-Host "Notepad++:      $root\notepadpp (install-notepadpp.ps1)"
