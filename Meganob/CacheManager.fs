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
    ArtifactType: string option
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

    let ensureCleanDirectory (path: AbsolutePath) =
        // Clear any existing directory to handle old cache entries being replaced
        if path.ExistsDirectory() then
            eprintfn $"Warning: Replacing outdated cache entry at '%s{path.Value}'"
            path.DeleteDirectoryRecursively()

        path.CreateDirectory()

    let readEntry (cacheDir: AbsolutePath): Task<CacheEntry option> = task {
        let metadataPath = cacheDir / cacheMetadataFileName
        if metadataPath.ExistsFile() then
            try
                let! json = metadataPath.ReadAllTextAsync()
                let doc = JsonDocument.Parse(json)
                let root = doc.RootElement
                let artifactType =
                    match root.TryGetProperty("ArtifactType") with
                    | true, prop when prop.ValueKind <> JsonValueKind.Null -> Some (prop.GetString())
                    | _ -> None
                return Some {
                    CacheKeyHash = root.GetProperty("CacheKeyHash").GetString()
                    LastAccessed = root.GetProperty("LastAccessed").GetDateTimeOffset()
                    ArtifactType = artifactType
                }
            with
            | _ -> return None
        else
            return None
    }

    let writeEntryJson (metadataPath: AbsolutePath) (cacheKeyHash: string) (lastAccessed: DateTimeOffset) (artifactType: string): Task<unit> = task {
        use stream = new MemoryStream()
        use writer = new Utf8JsonWriter(stream, JsonWriterOptions(Indented = true))
        writer.WriteStartObject()
        writer.WriteString("CacheKeyHash", cacheKeyHash)
        writer.WriteString("LastAccessed", lastAccessed)
        writer.WriteString("ArtifactType", artifactType)
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
                let! entry = readEntry cacheDir
                match entry with
                | Some e ->
                    match e.ArtifactType with
                    | Some artifactType ->
                        match config.ArtifactRegistry.TryGetLoader(artifactType) with
                        | Some loader ->
                            let! loaded = loader cacheDir
                            match loaded with
                            | Some artifact ->
                                // Update last accessed time
                                let metadataPath = cacheDir / cacheMetadataFileName
                                do! writeEntryJson metadataPath e.CacheKeyHash DateTimeOffset.UtcNow artifactType
                                return Some artifact
                            | None -> return None
                        | None ->
                            eprintfn $"Warning: Unknown artifact type '%s{artifactType}' in cache"
                            return None
                    | None ->
                        // Invalid cache entry without ArtifactType - treat as cache miss.
                        return None
                | None -> return None
        }

        member _.Store(inputs: IArtifact seq, output: IArtifact): Task = task {
            match! computeCacheKey inputs with
            | None -> ()  // Non-cacheable, skip
            | Some cacheKeyHash ->
                match output.CacheData with
                | None -> ()  // Output not storable
                | Some cacheable ->
                    match config.ArtifactRegistry.GetTypeName(output) with
                    | Some artifactType ->
                        let cacheDir = getCacheDirectory cacheKeyHash
                        ensureCleanDirectory cacheDir
                        do! cacheable.StoreTo(cacheDir)
                        let metadataPath = cacheDir / cacheMetadataFileName
                        do! writeEntryJson metadataPath cacheKeyHash DateTimeOffset.UtcNow artifactType
                    | None ->
                        let typeName = output.GetType().Name
                        eprintfn $"Warning: Artifact type '%s{typeName}' is not registered in cache config. Output will not be cached."
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
