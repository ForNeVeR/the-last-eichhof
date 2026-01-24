let licenseHeader = """
# SPDX-FileCopyrightText: 2024-2026 Friedrich von Never <friedrich@fornever.me>
#
# SPDX-License-Identifier: MIT

# This file is auto-generated.""".Trim()

#r "nuget: Generaptor.Library, 1.9.1"
open Generaptor
open Generaptor.GitHubActions
open type Generaptor.GitHubActions.Commands

let workflows = [
    workflow "main" [
        header licenseHeader
        name "Main"
        onPushTo "main"
        onPullRequestTo "main"
        onSchedule "0 0 * * 6"
        onWorkflowDispatch

        let dotNetJob name body = job name [
            setEnv "DOTNET_CLI_TELEMETRY_OPTOUT" "1"
            setEnv "DOTNET_NOLOGO" "1"
            setEnv "NUGET_PACKAGES" "${{ github.workspace }}/.github/nuget-packages"
            step(
                name = "Check out the sources",
                usesSpec = Auto "actions/checkout"
            )
            step(
                name = "Set up .NET SDK",
                usesSpec = Auto "actions/setup-dotnet"
            )

            yield! body
        ]

        dotNetJob "verify-workflows" [
            runsOn "ubuntu-24.04"
            step(
                run = "dotnet fsi ./scripts/github-actions.fsx verify"
            )
        ]

        let runOnAllImages = [
            strategy(failFast = false, matrix = [
                "image", [
                    "macos-15"
                    "ubuntu-24.04"
                    "ubuntu-24.04-arm"
                    "windows-11-arm"
                    "windows-2025"
                ]
            ])
            runsOn "${{ matrix.image }}"
        ]

        let pwsh(name, command) =
            step(
                name = name,
                shell = "pwsh",
                run = command
            )

        dotNetJob "meganob-tests" [
            yield! runOnAllImages
            pwsh(
                "Run tests",
                "dotnet test"
            )
        ]

        dotNetJob "build" [
            yield! runOnAllImages

            pwsh(
                "Install DOSBox-X",
                "scripts/Install-DOSBox-X.ps1"
            )
            pwsh(
                "Build the game",
                "dotnet run --project Build"
            )
        ]
    ]
]

exit <| EntryPoint.Process fsi.CommandLineArgs workflows
