module Build.DosBoxX

open System
open System.IO
open System.Text.RegularExpressions
open System.Threading.Tasks
open Medallion.Shell
open Meganob
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

let private RunCommand(dosBox: AbsolutePath, command: string, logger: string -> unit) = task {
    let logFile = Temporary.CreateTempFile()

    let! result = RunProcess(dosBox, [|
        "-noconfig"
        "-set"; $"log logfile=%s{logFile.Value}"
        "-set"; "dos log console=quiet" // keep only the terminal output in the log file
        "-c"; command
        "-silent" // exit after command termination
    |])

    logger $"Exit code: %d{result.ExitCode}"
    if not result.Success then
        logger $"Command standard output:\n%s{result.StandardOutput}"
        logger $"Command standard error:\n%s{result.StandardError}"
        failwith $"Cannot execute command \"{command}\". Exit code from {dosBox.FileName}: {result.ExitCode}."

    let! output = logFile.ReadAllTextAsync()
    return output
}

let GetVersion(context: IDependencyContext): Task<string> = task {
    let reporter = context.Reporter

    reporter.Status "Searching for executable"
    let dosBox = FindExecutable()
    match dosBox with
    | None -> return failwithf "Cannot find DOSBox-X executable."
    | Some dosBox ->
        let! output = RunCommand(dosBox, "ver", reporter.Log)
        let matchResult = Regex("(?:^|\n)DOSBox.+?version (.+?). Reported").Match output
        if not matchResult.Success then
            failwithf "Cannot parse DOSBox-X version."
        return matchResult.Groups[1].Value
}
