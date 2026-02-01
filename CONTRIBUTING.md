<!--
SPDX-FileCopyrightText: 2024-2026 Friedrich von Never <friedrich@fornever.me>

SPDX-License-Identifier: MIT
-->
Contributor Guide
=================

Prerequisites
-------------
To work with the project, you'll need the following software:
- [.NET SDK 10][dotnet-sdk] or later,
- [DOSBox-X][dosbox-x].

Build
-----
To build the game, use the following command:
```console
$ dotnet run --project Build
```

It will produce DOS-compatible game binary and resource bundle in the `out` folder.

File Encoding Changes
---------------------
If the automation asks you to update the file encoding (line endings or UTF-8 BOM) in certain files, run the following PowerShell script ([PowerShell Core][powershell] is recommended to run this script):
```console
$ pwsh -c "Install-Module VerifyEncoding -Repository PSGallery -RequiredVersion 2.2.1 -Force && Test-Encoding -AutoFix"
```

The `-AutoFix` switch will automatically fix the encoding issues, and you'll only need to commit and push the changes.

GitHub Actions
--------------
If you want to update the GitHub Actions used in the project, edit the file that generated them: `scripts/github-actions.fsx`.

Then run the following shell command:
```console
$ dotnet fsi scripts/github-actions.fsx
```

[dosbox-x]: https://dosbox-x.com/
[dotnet-sdk]: https://dotnet.microsoft.com/en-us/download
[powershell]: https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell
