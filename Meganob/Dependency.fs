namespace Meganob

open System
open System.Net.Http
open System.Security.Cryptography
open System.Threading.Tasks
open TruePath
open TruePath.SystemIo

[<Interface>]
type IDependencyContext =
    abstract member Reporter: IReporter
    abstract member TempFolder: AbsolutePath

[<Interface>]
type internal IBuildContext =
    abstract member Reporter: IReporter

    abstract member StoreDependency: dependency: IDependency * context: IDependencyContext -> Task

and [<Interface>] IDependency =
    abstract member GetForStore: IDependencyContext -> Task<obj>

[<Interface>]
type IDependency<'T> =
    abstract member Get: IDependencyContext -> Task<'T>
    inherit IDependency

module internal DependencyContext =
    let Of(bc: IBuildContext) =
        let tempFolder = lazy Temporary.CreateTempFolder("meganob")
        {
            new IDependencyContext with
                member _.Reporter = bc.Reporter
                member _.TempFolder = tempFolder.Value
        }

module Dependency =
    let Async<'a>(getter: IDependencyContext -> Task<'a>): IDependency<'a> = {
        new IDependency<'a> with
            member _.Get c = getter c
            member _.GetForStore c= task {
                let! r = getter c
                return box r
            }
    }

    let private AssertCorrectHash(file: AbsolutePath, expectedHash: Hash) =
        match expectedHash with
        | Sha256 sha256 -> task {
            use stream = file.OpenRead()
            use hasher = SHA256.Create()
            let! actualHash = hasher.ComputeHashAsync stream
            let actualHash = Convert.ToHexStringLower actualHash
            if sha256.ToLowerInvariant() <> actualHash then
                failwithf $"SHA256 of {file.FileName} was expected to be {expectedHash} but was {actualHash}."
        }

    let DownloadFile(uri: Uri, hash: Hash): IDependency<AbsolutePath> =
        Async(fun context -> task {
            let reporter = context.Reporter

            let fileName = uri.AbsolutePath.Split('/') |> Array.last
            let destination = context.TempFolder / fileName
            use client = new HttpClient()

            reporter.Status $"Connecting to {uri.Authority}"
            let! response = client.GetAsync uri
            response.EnsureSuccessStatusCode() |> ignore

            reporter.Status $"Downloading {uri}"
            let download = fun (byteProgress: IProgressReporter) -> task {
                let mutable totalBytes = 0L
                do! task {
                    use! readStream = response.Content.ReadAsStreamAsync()
                    use writeStream = destination.OpenWrite()

                    let buffer = Array.zeroCreate<byte> 8192
                    let mutable bytesRead = 0
                    let mutable continueReading = true
                    while continueReading do
                        let! read = readStream.ReadAsync(buffer, 0, buffer.Length)
                        bytesRead <- read
                        if bytesRead > 0 then
                            do! writeStream.WriteAsync(buffer, 0, bytesRead)
                            totalBytes <- totalBytes + int64 bytesRead
                            byteProgress.Report totalBytes
                        else
                            continueReading <- false
                }

                do! AssertCorrectHash(destination, hash)

                reporter.Log
                    $"File \"{fileName}\" ({string totalBytes} bytes) stored at \"{destination.Value}\"."
                return destination
            }
            match response.Content.Headers.ContentLength with
            | NullV -> return! download ProgressReporter.Null
            | NonNullV(contentLength) -> return! context.Reporter.WithProgress(
                $"Download {fileName}",
                contentLength,
                download
            )
        })
