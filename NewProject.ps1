# =====================================================================
#  NewProject.ps1 - Create a new STM32F103VET6 SPL project from template
#  Usage:  .\NewProject.ps1 -Name MyProject
# =====================================================================
param(
    [Parameter(Mandatory = $true)]
    [string]$Name
)

$ErrorActionPreference = "Stop"
$root     = $PSScriptRoot
$template = Join-Path $root "STM32F103VE_SPL_Template"
$target   = Join-Path $root $Name

if (-not (Test-Path $template)) {
    Write-Error "Template not found: $template"
    exit 1
}
if (Test-Path $target) {
    Write-Error "Target already exists: $target"
    exit 1
}

# 1) Copy template
Copy-Item -Path $template -Destination $target -Recurse

# 2) Remove generated build/cache folders
foreach ($d in @(".pio", ".vscode")) {
    $p = Join-Path $target $d
    if (Test-Path $p) { Remove-Item -Path $p -Recurse -Force }
}

Write-Host "Project created: $target" -ForegroundColor Green

# 3) Open in VSCode if available
$codeCandidates = @(
    "D:\program\Microsoft VS Code\bin\code.cmd",
    "$env:LOCALAPPDATA\Programs\Microsoft VS Code\bin\code.cmd"
)
$code = $codeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($code) {
    & $code $target
    Write-Host "Opened in VSCode." -ForegroundColor Green
} else {
    Write-Host "Please open manually in VSCode: $target" -ForegroundColor Yellow
}
