let licenseHeader = """
# SPDX-FileCopyrightText: 2024-2026 Friedrich von Never <friedrich@fornever.me>
#
# SPDX-License-Identifier: MIT

# This file is auto-generated.""".Trim()

#r "nuget: Generaptor.Library, 1.9.0"
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
                usesSpec = Auto "actions/checkout"
            )
            step(
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

        dotNetJob "build" [
            strategy(failFast = false, matrix = [
                "image", [
                    "macos-14"
                    "ubuntu-24.04"
                    "ubuntu-24.04-arm"
                    "windows-11-arm"
                    "windows-2025"
                ]
            ])
            runsOn "${{ matrix.image }}"

            step(
                name = "Install DOSBox-X",
                shell = "pwsh",
                run = "scripts/Install-DOSBox-X.ps1"
            )
        ]
    ]
]

exit <| EntryPoint.Process fsi.CommandLineArgs workflows
