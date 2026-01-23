# SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
#
# SPDX-License-Identifier: MIT

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($IsLinux)
{
    Write-Host 'Installing DOSBox-X on Linux using APT…'
    sudo apt-get update
    sudo apt-get install -y dosbox-x
}
elseif ($IsMacOS)
{
    Write-Host 'Installing DOSBox-X on macOS using Homebrew…'
    brew install dosbox-x
}
elseif ($IsWindows)
{
    Write-Host 'Installing DOSBox-X on Windows using WinGet...'
    winget install --id joncampbell123.DOSBox-X -e
}
else
{
    throw 'Unsupported platform'
}

Write-Host 'DOSBox-X installation completed successfully.'
