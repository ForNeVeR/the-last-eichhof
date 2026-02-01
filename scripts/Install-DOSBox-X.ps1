# SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
#
# SPDX-License-Identifier: MIT

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($IsLinux)
{
    Write-Host 'Installing DOSBox-X on Linux using APT…'
    sudo apt-get update
    if (!$?) { throw "Error from apt-get: $LASTEXITCODE." }
    sudo apt-get install -y dosbox-x
    if (!$?) { throw "Error from apt-get: $LASTEXITCODE." }
}
elseif ($IsMacOS)
{
    Write-Host 'Installing DOSBox-X on macOS using Homebrew…'
    brew install dosbox-x
    if (!$?) { throw "Error from brew: $LASTEXITCODE." }
}
elseif ($IsWindows)
{
    Write-Host 'Installing DOSBox-X on Windows using WinGet…'
    winget install --accept-source-agreements --id joncampbell123.DOSBox-X -e
    if (!$?) { throw "Error from winget: $LASTEXITCODE." }
}
else
{
    throw 'Unsupported platform.'
}

Write-Host 'DOSBox-X installation completed successfully.'
