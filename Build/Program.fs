open System.Threading.Tasks
open Build

module ExitCodes =
    let Success = 0
    let DosBoxXNotFound = 1

let private verifyEnvironment() = task {
    let dosBox = DosBoxX.FindExecutable()
    match dosBox with
    | None ->
        printfn "Cannot find DOSBox-X executable file."
        return ExitCodes.DosBoxXNotFound
    | Some db ->
        printfn $"Found DOSBox-X executable file: \"%s{db.Value}\"."
        do! DosBoxX.RunCommand(db, "ver", printfn "%s")
        return ExitCodes.Success
}

let private runSynchronously(task: Task<_>) =
    task.GetAwaiter().GetResult()

[<EntryPoint>]
let main(_: string[]): int =
    verifyEnvironment() |> runSynchronously
