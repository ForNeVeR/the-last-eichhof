namespace Meganob

open System
open System.Collections.Immutable
open System.Diagnostics
open System.IO
open System.Net.Http
open System.Security.Cryptography
open System.Text.Json
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
    abstract member DisplayName: string
    abstract member GetForStore: IDependencyContext -> Task<obj>
    /// Cache key for this dependency, or None if not cacheable
    abstract member CacheKey: ISerializableKey option
    /// Store value to cache directory. Only called for cacheable dependencies.
    abstract member StoreToCache: cacheDir: AbsolutePath * value: obj -> Task<unit>
    /// Load value from cache directory. Returns None if cache invalid/missing.
    abstract member LoadFromCache: cacheDir: AbsolutePath -> Task<obj option>

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

type internal DownloadFileKey(uri: Uri, hash: Hash) =
    interface ISerializableKey with
        member _.TypeDiscriminator = "Meganob.DownloadFileKey"
        member _.ToJson() =
            let hashStr =
                match hash with
                | Sha256 s -> $"SHA256:{s}"
            JsonSerializer.SerializeToElement({| uri = uri.ToString(); hash = hashStr |})

module Dependency =
    let NonCacheable<'a>(name: string, getter: IDependencyContext -> Task<'a>): IDependency<'a> = {
        new IDependency<'a> with
            member _.DisplayName = name
            member _.Get c = getter c
            member _.GetForStore c = task {
                let! r = getter c
                return box r
            }
            member _.CacheKey = None
            member _.StoreToCache(_, _) = task { () }
            member _.LoadFromCache _ = Task.FromResult None
    }

    let Cacheable<'a>(
        name: string,
        key: ISerializableKey,
        getter: IDependencyContext -> Task<'a>,
        storeToCache: AbsolutePath -> 'a -> Task<unit>,
        loadFromCache: AbsolutePath -> Task<'a option>
    ): IDependency<'a> = {
        new IDependency<'a> with
            member _.DisplayName = name
            member _.Get c = getter c
            member _.GetForStore c = task {
                let! r = getter c
                return box r
            }
            member _.CacheKey = Some key
            member _.StoreToCache(cacheDir, value) =
                storeToCache cacheDir (value :?> 'a)
            member _.LoadFromCache cacheDir = task {
                let! result = loadFromCache cacheDir
                return result |> Option.map box
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

    let private DownloadFileCore(uri: Uri, hash: Hash, context: IDependencyContext): Task<AbsolutePath> = task {
        let reporter = context.Reporter
        let stopwatch = Stopwatch.StartNew()

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
            stopwatch.Stop()

            reporter.Log
                $"File \"{fileName}\" ({string totalBytes} bytes) stored at \"{destination.Value}\" in %.2f{stopwatch.Elapsed.TotalSeconds}s."
            return destination
        }
        match response.Content.Headers.ContentLength with
        | NullV -> return! download ProgressReporter.Null
        | NonNullV(contentLength) -> return! context.Reporter.WithProgress(
            $"Download {fileName}",
            contentLength,
            download
        )
    }

    let DownloadFile(uri: Uri, hash: Hash): IDependency<AbsolutePath> =
        let key = DownloadFileKey(uri, hash)
        let fileName = uri.AbsolutePath.Split('/') |> Array.last

        let storeToCache (cacheDir: AbsolutePath) (downloadedPath: AbsolutePath) = task {
            let cachedFile = cacheDir / fileName
            File.Copy(downloadedPath.Value, cachedFile.Value, true)
        }

        let loadFromCache(cacheDir: AbsolutePath) = task {
            let cachedFile = cacheDir / fileName
            if cachedFile.ExistsFile() then
                return Some cachedFile
            else
                return None
        }

        Cacheable(
            $"Download {fileName} from {uri.Authority}",
            key,
            (fun context -> DownloadFileCore(uri, hash, context)),
            storeToCache,
            loadFromCache
        )

    let CollectFiles(name: string, folder: AbsolutePath, patterns: LocalPathPattern seq): IDependency<ImmutableArray<AbsolutePath>> =
        NonCacheable(name, fun _ -> task {
            // TODO: Provide file hashes as the cache key. We don't need actual caching here, though.
            return
                patterns
                |> Seq.collect(fun p -> folder.GetFiles p.Value)
                |> Seq.map AbsolutePath
                |> ImmutableArray.ToImmutableArray
        })
