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

This will use a downloaded Borland C++ 3.1 compiler in an isolated DOSBox-X environment.

It will produce a DOS-compatible game binary and resource bundle in the `out` folder.

### Manual Build
1. Make sure you have Borland C++ installed in `C:\BC` in your DOSBox (otherwise, you'll need to change the include and library paths in the `.PRJ` file).
2. `BC BALLER.PRJ`
3. **Build All**.

License Automation
------------------
If the CI asks you to update the file licenses, follow one of these:
1. Update the headers manually (look at the existing files), something like this:
   ```fsharp
   // SPDX-FileCopyrightText: %year% %your name% <%your contact info, e.g. email%>
   //
   // SPDX-License-Identifier: MIT
   ```
   (accommodate to the file's comment style if required).
2. Alternately, use the [REUSE][reuse] tool:
   ```console
   $ reuse annotate --license MIT --copyright '%your name% <%your contact info, e.g. email%>' %file names to annotate%
   ```

(Feel free to attribute the changes to "The Last Eichhof contributors <https://github.com/ForNeVeR/the-last-eichhof>" instead of your name in a multi-author file, or if you don't want your name to be mentioned in the project's source: this doesn't mean you'll lose the copyright.)

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
