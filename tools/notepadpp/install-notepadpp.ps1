<#
.SYNOPSIS
  Instaluje obsługę HP PPL w Notepad++: kolorowanie (UDL), autouzupełnianie i ppl.exe do sprawdzania błędów.

.PARAMETER PplExe
  Ścieżka do ppl.exe (domyślnie ..\build\ppl.exe albo ppl.exe obok skryptu).

.PARAMETER DryRun
  Pokazuje, co zostanie skopiowane, bez zmieniania plików.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File install-notepadpp.ps1
#>
param(
    [string]$PplExe = "",
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $PplExe) {
    foreach ($c in @((Join-Path $here 'ppl.exe'), (Join-Path $here '..\build\ppl.exe'))) {
        if (Test-Path $c) { $PplExe = (Resolve-Path $c).Path; break }
    }
}
if (-not $PplExe -or -not (Test-Path $PplExe)) { throw "Nie znaleziono ppl.exe. Podaj -PplExe ŚCIEŻKA." }

$nppUser = Join-Path $env:APPDATA 'Notepad++'
$nppDir = @("$env:ProgramFiles\Notepad++", "${env:ProgramFiles(x86)}\Notepad++") | Where-Object { Test-Path (Join-Path $_ 'notepad++.exe') } | Select-Object -First 1
$toolDir = Join-Path $env:LOCALAPPDATA 'hp-ppl'

$copies = @(
    @{ From = (Join-Path $here 'HP_PPL_udl.xml'); To = (Join-Path $nppUser 'userDefineLangs\HP_PPL_udl.xml') },
    @{ From = $PplExe; To = (Join-Path $toolDir 'ppl.exe') }
)
if ($nppDir) {
    $copies += @{ From = (Join-Path $here 'HP PPL.xml'); To = (Join-Path $nppDir 'autoCompletion\HP PPL.xml'); Admin = $true }
} else {
    Write-Warning "Nie znaleziono katalogu instalacji Notepad++ — pomijam plik autouzupełniania."
}

foreach ($c in $copies) {
    Write-Host ("{0}`n   -> {1}" -f $c.From, $c.To)
    if ($DryRun) { continue }
    try {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $c.To) | Out-Null
        Copy-Item -LiteralPath $c.From -Destination $c.To -Force
    } catch {
        if ($c.Admin) {
            Write-Warning "Brak uprawnień do $($c.To). Uruchom PowerShell jako administrator albo skopiuj plik ręcznie."
        } else { throw }
    }
}

$script = @"
NPP_SAVE
"`$(SYS.LOCALAPPDATA)\hp-ppl\ppl.exe" check "`$(FULL_CURRENT_PATH)"
"@
if (-not $DryRun) { Set-Content -Path (Join-Path $toolDir 'nppexec-ppl-check.txt') -Value $script -Encoding UTF8 }

Write-Host ""
Write-Host "Gotowe$(if ($DryRun) { ' (tryb próbny — nic nie zmieniono)' })."
Write-Host "Uruchom ponownie Notepad++. Język: menu Język > HP PPL (pliki .hpppl / .ppl wybiorą się same)."
Write-Host "Sprawdzanie błędów przez NppExec: zobacz tools\notepadpp\README.md."
