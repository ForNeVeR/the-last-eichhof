<!--
SPDX-FileCopyrightText: 2024-2026 Friedrich von Never <friedrich@fornever.me>

SPDX-License-Identifier: MIT
-->
Contributor Guide
=================

Prerequisites
-------------
To work with the project, you'll need the following software:
- [Bazel][bazel] (via [Bazelisk][bazelisk]),
- [DOSBox-X][dosbox-x].

For working with the build system or the CI workflow script (`scripts/github-actions.fsx`), you'll also need [.NET SDK 10][dotnet-sdk].

Build
-----
To build the game, use the following command:
```console
$ bazel build //:game
```

This will use a downloaded Borland C++ 3.1 compiler in an isolated DOSBox-X environment.

The game binary (`BALLER.EXE`) will be produced in `bazel-bin/`, and the resource bundle (`BEER.DAT`) will be available as part of the `:game` target.

### Manual Build
1. Make sure you have Borland C++ installed in `C:\BC` in your DOSBox (otherwise, you'll need to change the include and library paths in the `.PRJ` file).
2. `BC BALLER.PRJ`
3. **Build All**.

NuGet Dependencies
------------------
The F# build tool (`Build/`) uses [Paket][paket] and `paket2bazel` to manage NuGet dependencies. If you change `paket.dependencies`, you need to regenerate the Bazel dependency files:

1. Edit `paket.dependencies` as needed.
2. Run Paket to update the lock file:
   ```console
   $ dotnet paket install
   ```
3. Regenerate the Bazel files:
   ```console
   $ bazelisk run @rules_dotnet//tools/paket2bazel -- --dependencies-file $(pwd)/paket.dependencies --output-folder $(pwd)
   ```

This regenerates `paket.main.bzl` and `paket.main_extension.bzl`. Commit these generated files along with your changes.

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

[bazel]: https://bazel.build/
[bazelisk]: https://github.com/bazelbuild/bazelisk
[dosbox-x]: https://dosbox-x.com/
[dotnet-sdk]: https://dotnet.microsoft.com/en-us/download
[paket]: https://fsprojects.github.io/Paket/
[powershell]: https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell
