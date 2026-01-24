open System
open System.Threading.Tasks
open Build
open Meganob

let private RunSynchronously(task: Task<_>) =
    task.GetAwaiter().GetResult()

let private dosBoxVersion = Dependency.Async(DosBoxX.GetVersion)
let private borlandCppArtifact = Dependency.DownloadFile(
    Uri "https://archive.org/download/bcpp31/BCPP31.ZIP",
    Sha256 "CEAA8852DD2EE33AEDD471595051931BF96B44EFEE2AA2CD3706E41E38426F84"
)
let private downloadBorlandCpp = Target.OfDependencies [ borlandCppArtifact ]

let private targets = Map.ofArray [|
    "build", Target.OfDependencies(dosBoxVersion, borlandCppArtifact)
|]

[<EntryPoint>]
let main(args: string[]): int =
    BuildSystem.Run(targets, args) |> RunSynchronously
