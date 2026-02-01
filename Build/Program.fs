open System
open System.Collections.Immutable
open System.IO
open System.Reflection
open System.Threading.Tasks
open Build
open Meganob
open Spectre.Console
open TruePath
open TruePath.SystemIo

let private RunSynchronously(task: Task<_>) =
    task.GetAwaiter().GetResult()

// Define the build tasks
let private sourceFolder =
    let start = AbsolutePath(Assembly.GetExecutingAssembly().Location).Parent.Value
    let mutable current = start
    while current.Parent.HasValue && not <| (current / "TheLastEichhof.slnx").ExistsFile() do
        current <- current.Parent.Value
    if not <| (current / "TheLastEichhof.slnx").ExistsFile() then
        failwithf $"Cannot find folder containing the solution file, started from \"%s{current.Value}\"."
    current

// DOSBox-X version task (non-cacheable)
let dosBoxVersionTask: BuildTask = {
    Name = "DOSBox-X version"
    Inputs = ImmutableArray.Empty
    CacheData = None
    Execute = fun _ -> task {
        let! version = DosBoxX.GetVersion()
        let hash = CacheKey.ComputeCombinedHash [version]
        return ValueArtifact(version, hash) :> IArtifact
    }
}

let borlandCppDownloadTask =
    Tasks.DownloadFile
        (Uri "https://archive.org/download/bcpp31/BCPP31.ZIP")
        (Sha256 "CEAA8852DD2EE33AEDD471595051931BF96B44EFEE2AA2CD3706E41E38426F84")
let bcpp = Tasks.ExtractArchive borlandCppDownloadTask

let collectSources = Tasks.CollectFiles sourceFolder ([
    "*.ASH"
    "*.ASM"
    "*.C"
    "*.DEF"
    "*.H"
    "*.PRJ"
] |> Seq.map LocalPathPattern)

let prepareWorkspace = {
    Name = "prepare workspace"
    Inputs = ImmutableArray.Create(bcpp, collectSources)
    CacheData = Some <| DirectoryResult.CacheData "prepareWorkspace"
    Execute = fun (context, inputs) -> task {
        let inputs = inputs |> Seq.toArray
        let bcpp = inputs[0] :?> DirectoryResult
        let sources = inputs[1] :?> InputFileSet

        let reporter = context.Reporter
        reporter.Status "Preparing the workspace folder"

        let mutable entries = 0
        let diskRoot = Temporary.CreateTempFolder()
        for file in bcpp.Path.GetFiles("*", SearchOption.AllDirectories) |> Seq.map AbsolutePath do
            let relativePath = file.RelativeTo bcpp.Path
            let target = diskRoot / "BC" / relativePath
            target.Parent.Value.CreateDirectory()
            file.Copy target
            entries <- entries + 1

        let mutable sourceEntries = 0
        for source in sources.Files do
            let dest = diskRoot / "SRC" / source.RelativeTo sourceFolder
            dest.Parent.Value.CreateDirectory()
            source.Copy dest
            entries <- entries + 1
            sourceEntries <- sourceEntries + 1

        reporter.Log $"Prepared a workspace with {entries} entries ({sourceEntries} source files)."

        return DirectoryResult(diskRoot)
    }
}

let exe: BuildTask = {
    Name = "exe"
    Inputs = ImmutableArray.Create(dosBoxVersionTask, prepareWorkspace)
    CacheData = Some (FileResult.CacheData "build.v1")
    Execute = fun (context, inputs) -> task {
        let silent = true
        let inputs = inputs |> Seq.toArray
        let workspace = inputs[1]

        let workspace = workspace :?> DirectoryResult

        let workDir = Temporary.CreateTempFolder "tle"
        for source in workspace.Path.GetFiles("*", SearchOption.AllDirectories) |> Seq.map AbsolutePath do
            let dest = workDir / source.RelativeTo workspace.Path
            dest.Parent.Value.CreateDirectory()
            source.Copy dest

        let dosBox = DosBoxX.FindExecutable()
        match dosBox with
        | None -> return failwithf "Cannot find DOSBox-X executable."
        | Some dosBox ->

        let reporter = context.Reporter
        reporter.Status "Calling BC compiler in DOSBox-X"
        let! output = DosBoxX.RunCommands(dosBox, [
            $"MOUNT C \"%s{workDir.Value}\""
            @"SET PATH=C:\BC\BIN;%PATH%"
            @"C:"
            @"CD SRC"
            @"BC /b BALLER.PRJ"
        ], silent, reporter.Log)
        reporter.Log $"# Compiler output:\n{Markup.Escape output}"

        let baller = workDir / "SRC" / "BALLER.EXE"
        if not <| baller.ExistsFile() then
            failwithf $"File \"%s{baller.Value}\" not found. Check the compilation log for details."

        return FileResult baller :> IArtifact
    }
}

let build: BuildTask = {
    Name = "build"
    Inputs = ImmutableArray.Create(exe)
    CacheData = None
    Execute = fun (context, inputs) -> task {
        let exe = inputs |> Seq.exactlyOne :?> FileResult
        let exe = exe.Path
        let outputFolder = sourceFolder / "out"
        outputFolder.CreateDirectory()
        let target = outputFolder / exe.FileName
        exe.Copy(target, overwrite = true)

        context.Reporter.Log $"Output saved successfully to \"{target}\"."

        return NullResult()
    }
}

let private tasks = [bcpp; build ]

[<EntryPoint>]
let main(args: string[]): int =
    BuildSystem.Run(tasks, args, Some CacheConfig.Default) |> RunSynchronously
