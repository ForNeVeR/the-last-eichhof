open System
open System.Collections.Immutable
open System.Reflection
open System.Threading.Tasks
open Build
open Meganob
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
    Id = Guid.NewGuid()
    Name = "DOSBox-X version"
    Inputs = ImmutableArray.Empty
    CacheData = None  // Non-cacheable: always checks current version
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

let collectSources = Tasks.CollectFiles sourceFolder [LocalPathPattern "*.PRJ"]

let build: BuildTask = {
    Id = Guid.NewGuid()
    Name = "build"
    Inputs = ImmutableArray.Create(dosBoxVersionTask, bcpp, collectSources)
    CacheData = Some (FileResult.CacheData "build.v1")
    Execute = fun (context, inputs) -> task {
        let [|_dosBox; bcpp; sources|] = inputs |> Seq.toArray

        let bcpp = bcpp :?> DirectoryResult
        let sources = sources :?> InputFileSet

        let workDir = Temporary.CreateTempFolder "tle"
        for source in sources.Files do
            let dest = workDir / source.RelativeTo sourceFolder
            dest.Parent.Value.CreateDirectory()
            source.Copy dest

        let dosBox = DosBoxX.FindExecutable()
        match dosBox with
        | None -> return failwithf "Cannot find DOSBox-X executable."
        | Some dosBox ->

        let reporter = context.Reporter
        reporter.Status "Calling BC compiler in DOSBox-X"
        let! output = DosBoxX.RunCommands(dosBox, [
            $"MOUNT B \"%s{bcpp.Path.Value}\" -ro"
            $"MOUNT S \"%s{workDir.Value}\""
            @"SET PATH=B:\BIN;%PATH%"
            @"S:"
            @"BC BALLER.PRJ"
        ], reporter.Log)
        reporter.Log $"# Compiler output:\n{output}"

        let baller = workDir / "BALLER.EXE"
        if not <| baller.ExistsFile() then
            failwithf $"File \"%s{baller.Value}\" not found. Check the compilation log for details."

        return FileResult baller :> IArtifact
    }
}

let private tasks = [bcpp; build ]

[<EntryPoint>]
let main(args: string[]): int =
    BuildSystem.Run(tasks, args, Some CacheConfig.Default) |> RunSynchronously
