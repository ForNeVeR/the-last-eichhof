module Meganob.Tasks

open System
open System.Collections.Immutable
open System.Diagnostics
open System.IO.Compression
open System.Net.Http
open System.Security.Cryptography
open System.Threading.Tasks
open TruePath
open TruePath.SystemIo

let DownloadFile (uri: Uri) (hash: Hash): BuildTask =
    let fileName = uri.AbsolutePath.Split('/') |> Array.last
    let inputTask = {
        Name = $"Input spec for \"{fileName}\""
        Inputs = ImmutableArray.Empty
        CacheData = None
        Execute = fun _ -> Task.FromResult <| DownloadInput(uri, hash)
    }

    let assertCorrectHash(file: AbsolutePath, expectedHash: Hash) =
        match expectedHash with
        | Sha256 sha256 -> task {
            use stream = file.OpenRead()
            use hasher = SHA256.Create()
            let! actualHash = hasher.ComputeHashAsync stream
            let actualHash = Convert.ToHexStringLower actualHash
            if sha256.ToLowerInvariant() <> actualHash then
                failwithf $"SHA256 of {file.FileName} was expected to be {expectedHash} but was {actualHash}."
        }

    {
        Name = "Download file"
        Inputs = ImmutableArray.Create inputTask
        CacheData = Some (FileResult.CacheData "download.v1")
        Execute = fun(context, _) -> task {
            let reporter = context.Reporter
            let stopwatch = Stopwatch.StartNew()

            let fileName = uri.AbsolutePath.Split('/') |> Array.last
            let destination = Temporary.CreateTempFolder() / fileName
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

                do! assertCorrectHash(destination, hash)
                stopwatch.Stop()

                reporter.Log
                    $"File \"{fileName}\" ({string totalBytes} bytes) stored at \"{destination.Value}\" in %.2f{stopwatch.Elapsed.TotalSeconds}s."
                return FileResult destination :> IArtifact
            }
            match response.Content.Headers.ContentLength with
            | NullV -> return! download ProgressReporter.Null
            | NonNullV(contentLength) -> return! context.Reporter.WithProgress(
                $"Download {fileName}",
                contentLength,
                download
            )
        }
    }

let ExtractArchive(archive: BuildTask): BuildTask = {
    Name = "Extract archive"
    Inputs = ImmutableArray.Create(archive)
    CacheData = Some (DirectoryResult.CacheData "extract.v1")
    Execute = fun (_, inputs) -> task {
        let input = inputs |> Seq.exactlyOne :?> FileResult
        let destination = Temporary.CreateTempFolder()
        do! ZipFile.ExtractToDirectoryAsync(input.Path.Value, destination.Value)
        return DirectoryResult destination :> IArtifact
    }
}

let CollectFiles (sourceFolder: AbsolutePath) (patterns: LocalPathPattern seq): BuildTask = {
    Name = "Collect source files"
    Inputs = ImmutableArray.Empty
    CacheData = None  // Non-cacheable: always reads from disk
    Execute = fun _ -> task {
        let files =
            patterns
            |> Seq.collect (fun pattern ->
                sourceFolder.GetFiles pattern.Value
                |> Seq.map AbsolutePath
            )
            |> ImmutableArray.CreateRange
        return InputFileSet files :> IArtifact
    }
}
