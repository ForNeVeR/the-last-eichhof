namespace Meganob

open System
open System.Collections.Immutable
open System.IO
open System.Security.Cryptography
open System.Threading.Tasks
open Meganob
open TruePath
open TruePath.SystemIo

type ValueArtifact<'T>(value: 'T, hash: Hash) =
    member _.Value = value

    interface IArtifact with
        member _.GetContentHash() = Task.FromResult hash
        member _.CacheData = None  // Values aren't stored, just hashed

type FileResult(path: AbsolutePath) =
    member _.Path = path

    interface IArtifact with
        member _.GetContentHash() = FileResult.CalculateHash path
        member _.CacheData = Some { new ICacheable with
            member _.StoreTo(cacheDir) = task {
                let destPath = cacheDir / path.FileName
                path.Copy(destPath)
            }
            member _.LoadFrom(cacheDir) = task {
                let cached = cacheDir / path.FileName
                if cached.ExistsFile() then
                    return Some (FileResult(cached) :> IArtifact)
                else
                    return None
            }
        }

    static member internal CalculateHash(path: AbsolutePath) = task {
        use stream = path.OpenRead()
        let sha256 = SHA256.Create()
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
        let sha256 = SHA256.Create()
        let! bytes = sha256.ComputeHashAsync stream
        return Sha256(Convert.ToHexStringLower bytes)
    }

type DirectoryResult(directory: AbsolutePath) =
    member _.Path = directory

    interface IArtifact with
        member _.GetContentHash() = task {
            let allFiles = directory.GetFiles "*" |> Seq.map AbsolutePath
            return! FileResult.CalculateHash allFiles
        }
        member _.CacheData = Some { new ICacheable with
            member _.StoreTo(cacheDir) = task {
                let files = directory.GetFiles "*" |> Seq.map AbsolutePath
                let cacheTarget = cacheDir / "data"
                for file in files do
                    let relativePath = file.RelativeTo directory
                    let destPath = cacheTarget / relativePath.FileName
                    destPath.Parent.Value.CreateDirectory()
                    (AbsolutePath file).Copy(destPath)
            }
            member _.LoadFrom(cacheDir) =
                Task.FromResult(Some <| DirectoryResult(cacheDir / "data"))
        }

// Artifact for files, non-cacheable.
type InputFileSet(files: ImmutableArray<AbsolutePath>) =
    member _.Files = files

    interface IArtifact with
        member _.GetContentHash() = FileResult.CalculateHash files
        member _.CacheData = None // no caching of the file sets

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
        member _.CacheData = None
