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
Currently, only an action to verify the environment is available. To verify the environment, run the following command:
```console
$ dotnet run --project Build
```

It will output DOSBox-X version information, if available. In case of problems, it will print error information and exit with a non-zero exit code.

GitHub Actions
--------------
If you want to update the GitHub Actions used in the project, edit the file that generated them: `scripts/github-actions.fsx`.

Then run the following shell command:
```console
$ dotnet fsi scripts/github-actions.fsx
```

[dotnet-sdk]: https://dotnet.microsoft.com/en-us/download
[dosbox-x]: https://dosbox-x.com/
