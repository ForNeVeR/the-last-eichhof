namespace Meganob

open System
open System.IO
open System.Text.Json
open System.Threading.Tasks
open TruePath
open TruePath.SystemIo

type internal CacheEntry = {
    CacheKeyHash: string
    LastAccessed: DateTimeOffset
}

type CleanupResult = {
    EntriesRemoved: int
    BytesFreed: int64
}

type internal CacheManager(config: CacheConfig) =
    let cacheMetadataFileName = "cache.json"

    let computeCacheKey (inputs: IArtifact seq): Task<string option> =
        let inputsList = inputs |> Seq.toList
        if inputsList.IsEmpty then
            Task.FromResult None // No inputs = non-cacheable
        else
            task {
                let! hash = CacheKey.ComputeHash inputsList
                return Some hash
            }


    let getCacheDirectory (cacheKeyHash: string): AbsolutePath =
        config.CacheFolder / cacheKeyHash

    let ensureDirectoryExists (path: AbsolutePath) =
        Directory.CreateDirectory(path.Value) |> ignore

    let readEntry (cacheDir: AbsolutePath): Task<CacheEntry option> = task {
        let metadataPath = cacheDir / cacheMetadataFileName
        if metadataPath.ExistsFile() then
            try
                let! json = metadataPath.ReadAllTextAsync()
                let doc = JsonDocument.Parse(json)
                let root = doc.RootElement
                return Some {
                    CacheKeyHash = root.GetProperty("CacheKeyHash").GetString()
                    LastAccessed = root.GetProperty("LastAccessed").GetDateTimeOffset()
                }
            with
            | _ -> return None
        else
            return None
    }

    let writeEntryJson (metadataPath: AbsolutePath) (cacheKeyHash: string) (lastAccessed: DateTimeOffset): Task<unit> = task {
        use stream = new MemoryStream()
        use writer = new Utf8JsonWriter(stream, JsonWriterOptions(Indented = true))
        writer.WriteStartObject()
        writer.WriteString("CacheKeyHash", cacheKeyHash)
        writer.WriteString("LastAccessed", lastAccessed)
        writer.WriteEndObject()
        do! writer.FlushAsync()

        let json = Text.Encoding.UTF8.GetString(stream.ToArray())
        do! metadataPath.WriteAllTextAsync json
    }

    interface ICacheManager with
        member _.TryLoadCached(inputs: IArtifact seq): Task<IArtifact option> = task {
            match! computeCacheKey inputs with
            | None -> return None  // Non-cacheable
            | Some cacheKeyHash ->
                let cacheDir = getCacheDirectory cacheKeyHash
                let metadataPath = cacheDir / cacheMetadataFileName
                if metadataPath.ExistsFile() then
                    // Try to load from cache
                    let! entry = readEntry cacheDir
                    match entry with
                    | Some e ->
                        // Try to find an artifact that can load from this cache
                        // We need the artifact type to load. For now, try FileArtifact.
                        // In practice, we'll need a way to know the artifact type.
                        // For simplicity, check for files in the cache directory.
                        let files = Directory.GetFiles(cacheDir.Value)
                                    |> Array.filter (fun f -> Path.GetFileName(f) <> cacheMetadataFileName)
                        if files.Length > 0 then
                            // Update last accessed time
                            do! writeEntryJson metadataPath e.CacheKeyHash DateTimeOffset.UtcNow
                            // Return a placeholder - the actual loading depends on artifact type
                            // For FileArtifact, we return the cached file
                            let cachedFile = AbsolutePath(files[0])
                            let hash = Sha256 e.CacheKeyHash  // Use cache key as placeholder hash
                            return Some (FileResult(cachedFile) :> IArtifact)
                        else
                            return None
                    | None -> return None
                else
                    return None
        }

        member _.Store(inputs: IArtifact seq, output: IArtifact): Task = task {
            match! computeCacheKey inputs with
            | None -> ()  // Non-cacheable, skip
            | Some cacheKeyHash ->
                match output.CacheData with
                | None -> ()  // Output not storable
                | Some cacheable ->
                    let cacheDir = getCacheDirectory cacheKeyHash
                    ensureDirectoryExists cacheDir
                    do! cacheable.StoreTo(cacheDir)
                    let metadataPath = cacheDir / cacheMetadataFileName
                    do! writeEntryJson metadataPath cacheKeyHash DateTimeOffset.UtcNow
        }

    member _.Cleanup(verbose: bool): Task<CleanupResult> = task {
        let mutable entriesRemoved = 0
        let mutable bytesFreed = 0L

        if config.CacheFolder.ExistsDirectory() then
            if verbose then
                printfn $"Cache folder: %s{config.CacheFolder.Value}"

            let now = DateTimeOffset.UtcNow
            let directories = Directory.GetDirectories(config.CacheFolder.Value)
            for dir in directories do
                let cacheDir = AbsolutePath dir
                match! readEntry cacheDir with
                | Some entry ->
                    let age = now - entry.LastAccessed
                    if age > config.MaxAge then
                        let hash = Path.GetFileName(dir)

                        if verbose then
                            printfn $"  Removing: %s{hash}"

                        let dirInfo = DirectoryInfo(dir)
                        let size = dirInfo.EnumerateFiles("*", SearchOption.AllDirectories) |> Seq.sumBy _.Length
                        Directory.Delete(dir, true)
                        entriesRemoved <- entriesRemoved + 1
                        bytesFreed <- bytesFreed + size
                | None ->
                    // Invalid metadata directory, skip with warning
                    if verbose then
                        printfn $"  Warning: Invalid cache directory (no metadata): %s{dir}"

        return { EntriesRemoved = entriesRemoved; BytesFreed = bytesFreed }
    }
