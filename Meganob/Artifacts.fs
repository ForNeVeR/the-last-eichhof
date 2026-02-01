namespace Meganob

open System
open System.Collections.Immutable
open System.IO
open System.Security.Cryptography
open System.Threading.Tasks
open Meganob
open TruePath
open TruePath.SystemIo

type NullResult() =
    interface IArtifact with
        member _.GetContentHash() = Task.FromResult(
            Sha256 "0000000000000000000000000000000000000000000000000000000000000000"
        )

type ValueArtifact<'T>(value: 'T, hash: Hash) =
    member _.Value = value

    interface IArtifact with
        member _.GetContentHash() = Task.FromResult hash

type FileResult(path: AbsolutePath) =
    member _.Path = path

    interface IArtifact with
        member _.GetContentHash() = FileResult.CalculateHash path

    static member internal CalculateHash(path: AbsolutePath) = task {
        use stream = path.OpenRead()
        use sha256 = SHA256.Create()
        let! bytes = sha256.ComputeHashAsync stream
        return Sha256(Convert.ToHexStringLower bytes)
    }

    static member internal CalculateHash(files: AbsolutePath seq) = task {
        let! hashes = files |> Seq.map FileResult.CalculateHash |> Task.WhenAll
        let hashes = hashes |> Seq.sortBy _.ToHexString()
        use stream = new MemoryStream()
        for hash in hashes do
            do! stream.WriteAsync(hash.GetBytes())
        stream.Position <- 0L
        use sha256 = SHA256.Create()
        let! bytes = sha256.ComputeHashAsync stream
        return Sha256(Convert.ToHexStringLower bytes)
    }

module FileResult =
    /// Create cache data for tasks that output a single file
    let CacheData(version: string): TaskCacheData = {
        Version = version
        StoreTo = fun cacheDir output ->
            let file = (output :?> FileResult).Path
            file.Copy(cacheDir / file.FileName)
            Task.CompletedTask
        LoadFrom = fun cacheDir -> task {
            let files =
                Directory.GetFiles(cacheDir.Value)
                |> Array.filter (fun f -> Path.GetFileName(f) <> "cache.json")

            match files.Length with
            | 0 ->
                return None
            | 1 ->
                return Some (FileResult(AbsolutePath(files[0])) :> IArtifact)
            | count ->
                return raise (InvalidOperationException(
                    $"Expected exactly one cached file in '{cacheDir}', but found {count}."
                ))
        }
    }

type DirectoryResult(directory: AbsolutePath) =
    member _.Path = directory

    interface IArtifact with
        member _.GetContentHash() = task {
            let allFiles = directory.GetFiles("*", SearchOption.AllDirectories) |> Seq.map AbsolutePath
            return! FileResult.CalculateHash allFiles
        }

module DirectoryResult =
    /// Create cache data for tasks that output a directory
    let CacheData(version: string): TaskCacheData = {
        Version = version
        StoreTo = fun cacheDir output ->
            let dir = (output :?> DirectoryResult).Path
            let files = dir.GetFiles("*", SearchOption.AllDirectories) |> Seq.map AbsolutePath
            let cacheTarget = cacheDir / "data"
            for file in files do
                let relativePath = file.RelativeTo dir
                let destPath = cacheTarget / relativePath
                destPath.Parent.Value.CreateDirectory()
                file.Copy(destPath)
            Task.CompletedTask
        LoadFrom = fun cacheDir -> task {
            let dataDir = cacheDir / "data"
            if dataDir.ExistsDirectory() then
                return Some (DirectoryResult(dataDir) :> IArtifact)
            else
                return None
        }
    }

// Artifact for files, non-cacheable.
type InputFileSet(files: ImmutableArray<AbsolutePath>) =
    member _.Files = files

    interface IArtifact with
        member _.GetContentHash() = FileResult.CalculateHash files

/// Artifact for download specification (URL + expected hash).
type DownloadInput(uri: Uri, expectedHash: Hash) =
    static member Version = "v1"

    member _.Uri = uri
    member _.ExpectedHash = expectedHash

    interface IArtifact with
        member _.GetContentHash() =
            Task.FromResult (
                CacheKey.ComputeCombinedHash [
                    DownloadInput.Version
                    uri.ToString()
                    expectedHash.ToHexString()
                ]
            )
