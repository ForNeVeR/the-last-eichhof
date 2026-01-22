let licenseHeader = """
# SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
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

        job "verify-workflows" [
            runsOn "ubuntu-24.04"

            setEnv "DOTNET_CLI_TELEMETRY_OPTOUT" "1"
            setEnv "DOTNET_NOLOGO" "1"
            setEnv "NUGET_PACKAGES" "${{ github.workspace }}/.github/nuget-packages"
            step(
                usesSpec = Auto "actions/checkout"
            )
            step(
                usesSpec = Auto "actions/setup-dotnet"
            )
            step(
                run = "dotnet fsi ./scripts/github-actions.fsx verify"
            )
        ]
    ]
]

exit <| EntryPoint.Process fsi.CommandLineArgs workflows
