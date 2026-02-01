// SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
//
// SPDX-License-Identifier: MIT

module Build.DosBoxX

open System
open System.IO
open System.Text.RegularExpressions
open System.Threading.Tasks
open Medallion.Shell
open TruePath
open TruePath.SystemIo

let FindExecutable(): AbsolutePath option =
    let split(s: string) = s.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries)

    let extensions =
        (if OperatingSystem.IsWindows() then Environment.GetEnvironmentVariable "PATHEXT" else "") |> split
    let resolveExecutableFile(p: AbsolutePath) =
        if p.ExistsFile() // NOTE: Here we ignore check for exe bit on Unix for simplicity
        then Some p
        else
            extensions
            |> Seq.map p.WithExtension
            |> Seq.tryFind _.ExistsFile()

    let path = Environment.GetEnvironmentVariable "PATH"
    let folders = path.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries)
    let defaultLocations =
        if OperatingSystem.IsWindows() then [|AbsolutePath "C:\DOSBox-X"|]
        else Array.empty

    let allLocations =
        folders
        |> Seq.map LocalPath
        |> Seq.append(defaultLocations |> Seq.map LocalPath)

    allLocations
    |> Seq.map(fun p -> p / "dosbox-x")
    |> Seq.map _.ResolveToCurrentDirectory()
    |> Seq.choose resolveExecutableFile
    |> Seq.tryHead

let private RunProcess(executable: AbsolutePath, args: string seq): Task<CommandResult> =
    let args = args |> Seq.map box |> Seq.toArray
    Command.Run(executable.Value, arguments = args).Task

let RunCommands(dosBox: AbsolutePath, commands: string seq, silent: bool, logger: string -> unit) = task {
    let logFile = Temporary.CreateTempFile()

    let! result = RunProcess(dosBox, [|
        "-noconfig"
        "-set"; "cpu cycles=max"
        "-set"; "cpu turbo=true"
        "-set"; $"log logfile=%s{logFile.Value}"
        "-set"; "dos log console=quiet" // keep only the terminal output in the log file
        yield! commands |> Seq.collect(fun c -> ["-c"; c])
        if silent then "-silent" // exit after command termination
    |])

    logger $"Command exit code: %d{result.ExitCode}"
    if not result.Success then
        logger $"Command standard output:\n%s{result.StandardOutput}"
        logger $"Command standard error:\n%s{result.StandardError}"
        failwith $"Cannot execute DOSBox-X command. Exit code from {dosBox.FileName}: {result.ExitCode}."

    let! output = logFile.ReadAllTextAsync()
    return output
}

let GetVersion(): Task<string> = task {
    let dosBox = FindExecutable()
    match dosBox with
    | None -> return failwithf "Cannot find DOSBox-X executable."
    | Some dosBox ->
        let! output = RunCommands(dosBox, ["ver"], true, ignore)
        let matchResult = Regex("(?:^|\n)DOSBox.+?version (.+?). Reported").Match output
        if not matchResult.Success then
            failwithf "Cannot parse DOSBox-X version."
        let version = matchResult.Groups[1].Value
        return version
}
