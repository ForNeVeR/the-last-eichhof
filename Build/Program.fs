// SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
//
// SPDX-License-Identifier: MIT

open System
open System.IO
open Build
open TruePath
open TruePath.SystemIo

type Args = {
    BcppDir: string
    Output: string
    ProjectFile: string
    Sources: string list
}

let private parseArgs (argv: string[]): Args =
    let mutable bcppDir = ""
    let mutable output = ""
    let mutable projectFile = ""
    let sources = ResizeArray<string>()
    let mutable i = 0
    while i < argv.Length do
        match argv[i] with
        | "--bcpp-dir" ->
            i <- i + 1
            bcppDir <- argv[i]
        | "--output" ->
            i <- i + 1
            output <- argv[i]
        | "--project-file" ->
            i <- i + 1
            projectFile <- argv[i]
        | arg ->
            sources.Add(arg)
        i <- i + 1
    { BcppDir = bcppDir; Output = output; ProjectFile = projectFile; Sources = Seq.toList sources }

let private copyDirectory (source: AbsolutePath) (destPrefix: string) (root: AbsolutePath) =
    for file in source.GetFiles("*", SearchOption.AllDirectories) |> Seq.map AbsolutePath do
        let relativePath = file.RelativeTo source
        let target = root / destPrefix / relativePath
        target.Parent.Value.CreateDirectory()
        file.Copy target

[<EntryPoint>]
let main (argv: string[]): int =
    let args = parseArgs argv

    if String.IsNullOrEmpty args.BcppDir then
        eprintfn "Error: --bcpp-dir is required."
        1
    elif String.IsNullOrEmpty args.Output then
        eprintfn "Error: --output is required."
        1
    elif String.IsNullOrEmpty args.ProjectFile then
        eprintfn "Error: --project-file is required."
        1
    elif args.Sources.IsEmpty then
        eprintfn "Error: at least one source file is required."
        1
    else

    let tempDir = Temporary.CreateTempFolder()

    let absolutize(p: string) =
        AbsolutePath.CurrentWorkingDirectory / p

    try
        try
            // Copy BC++ files into temp/BC/
            let bcppDir = absolutize args.BcppDir
            copyDirectory bcppDir "BC" tempDir

            // Copy source files into temp/SRC/ (flat, just basenames)
            let srcDir = tempDir / "SRC"
            (AbsolutePath srcDir.Value).CreateDirectory()
            for source in args.Sources do
                let sourcePath = absolutize source
                let dest = srcDir / sourcePath.FileName
                sourcePath.Copy dest

            // Find DosBox-X
            let dosBox = DosBoxX.FindExecutable()
            match dosBox with
            | None ->
                eprintfn "Error: Cannot find DOSBox-X executable."
                1
            | Some dosBox ->

            // Run DosBox-X with BC compiler
            let result = DosBoxX.RunCommands(dosBox, [
                $"MOUNT C \"%s{tempDir.Value}\""
                @"SET PATH=C:\BC\BIN;%PATH%"
                @"C:"
                @"CD SRC"
                $"BC /b %s{args.ProjectFile}"
            ], true, eprintfn "%s")

            result.Wait()

            // Verify output EXE exists
            let exePath = srcDir / args.ProjectFile.Replace(".PRJ", ".EXE")
            if not <| exePath.ExistsFile() then
                eprintfn $"Error: Output file \"%s{exePath.Value}\" not found. Check the compilation log for details."
                1
            else

            // Copy to output path
            let outputPath = absolutize args.Output
            outputPath.Parent.Value.CreateDirectory()
            exePath.Copy(outputPath, overwrite = true)

            printfn $"Successfully compiled %s{outputPath.FileName}."
            0
        with ex ->
            eprintfn $"Error: %s{ex.Message}."
            1
    finally
        try tempDir.DeleteDirectoryRecursively() with _ -> ()
