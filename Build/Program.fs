open System
open System.Collections.Immutable
open System.IO.Compression
open System.Reflection
open System.Threading.Tasks
open Build
open Meganob
open TruePath
open TruePath.SystemIo

let private RunSynchronously(task: Task<_>) =
    task.GetAwaiter().GetResult()

let private dosBoxVersion = Dependency.NonCacheable("DOSBox-X version", DosBoxX.GetVersion)
let private borlandCppArtifact = Dependency.DownloadFile(
    Uri "https://archive.org/download/bcpp31/BCPP31.ZIP",
    Sha256 "CEAA8852DD2EE33AEDD471595051931BF96B44EFEE2AA2CD3706E41E38426F84"
)

let private extractArchive(context: ITargetContext): Task<ITargetResult> = task {
    let destination = Temporary.CreateTempFolder()
    let archive = context.GetDependency borlandCppArtifact :?> AbsolutePath
    do! ZipFile.ExtractToDirectoryAsync(archive.Value, destination.Value)
    return DirectoryResult destination :> ITargetResult
}

let private sourceFolder = lazy (
    let start = AbsolutePath(Assembly.GetExecutingAssembly().Location).Parent.Value
    let mutable current = start
    while current.Parent.HasValue && not <| (current / "TheLastEichhof.slnx").ExistsFile() do
        current <- current.Parent.Value
    if not <| (current / "TheLastEichhof.slnx").ExistsFile() then
        failwithf $"Cannot find folder containing the solution file, started from \"%s{current.Value}\"."
    current
)
let collectSources = Dependency.CollectFiles("sources", sourceFolder.Value, [
    // TODO: Add more files: *.ASM, *.ASH, *.C, *.CFG, *.DEF
    LocalPathPattern "*.PRJ"
])

let private buildSources (bcpp: Target) (context: ITargetContext): Task<ITargetResult> = task {
    let bcpp = context.GetDependency bcpp :?> DirectoryResult
    let sources = context.GetDependency collectSources :?> ImmutableArray<AbsolutePath>

    let workDir = Temporary.CreateTempFolder "tle"
    for source in sources do
        source.Copy workDir

    context.Reporter.Log $"Host source folder: \"%s{workDir.Value}\"."

    let dosBox = DosBoxX.FindExecutable()
    match dosBox with
    | None -> return failwithf "Cannot find DOSBox-X executable."
    | Some dosBox ->

    let! output = DosBoxX.RunCommands(dosBox, [
        $"MOUNT B \"%s{bcpp.Path.Value}\""
        $"MOUNT S \"%s{workDir.Value}\""
        @"SET PATH=B:\BIN;%PATH%"
        @"CD /D S:\"
        @"BC BALLER.PRJ"
    ], context.Reporter.Log)
    context.Reporter.Log output

    let baller = workDir / "BALLER.EXE"
    if not <| baller.ExistsFile() then
        failwithf $"File \"%s{baller.Value}\" not found. Check the compilation log for details."

    return FileResult baller :> ITargetResult
}

let private targets = Map.ofArray [|
    let bcpp = Target.Define("bcpp", [ borlandCppArtifact ], extractArchive)
    let build = Target.Define("build", [ dosBoxVersion; bcpp; collectSources ], buildSources bcpp)

    "bcpp", bcpp
    "build", build
|]

[<EntryPoint>]
let main(args: string[]): int =
    BuildSystem.Run(targets, args, Some CacheConfig.Default) |> RunSynchronously
