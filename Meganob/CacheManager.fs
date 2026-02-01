// SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
//
// SPDX-License-Identifier: MIT

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

    let computeCacheKey (inputs: IArtifact seq) (cacheData: TaskCacheData): Task<string option> =
        let inputsList = inputs |> Seq.toList
        if inputsList.IsEmpty then
            Task.FromResult None // No inputs = non-cacheable
        else
            task {
                let! inputHash = CacheKey.ComputeHash inputsList
                // Include task version in cache key to differentiate tasks with same inputs
                let combined = CacheKey.ComputeCombinedHash [inputHash; cacheData.Version]
                return Some (combined.ToHexString())
            }

    let getCacheDirectory (cacheKeyHash: string): AbsolutePath =
        config.CacheFolder / cacheKeyHash

    let ensureCleanDirectory (path: AbsolutePath) =
        // Clear any existing directory to handle old cache entries being replaced
        if path.ExistsDirectory() then
            printfn $"Warning: Replacing outdated cache entry at '%s{path.Value}'"
            path.DeleteDirectoryRecursively()

        path.CreateDirectory()

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
        member _.TryLoadCached(inputs: IArtifact seq, cacheData: TaskCacheData): Task<IArtifact option> = task {
            match! computeCacheKey inputs cacheData with
            | None -> return None  // Non-cacheable
            | Some cacheKeyHash ->
                let cacheDir = getCacheDirectory cacheKeyHash
                let! entry = readEntry cacheDir
                match entry with
                | Some e ->
                    let! loaded = cacheData.LoadFrom cacheDir
                    match loaded with
                    | Some artifact ->
                        // Update last accessed time
                        let metadataPath = cacheDir / cacheMetadataFileName
                        do! writeEntryJson metadataPath e.CacheKeyHash DateTimeOffset.UtcNow
                        return Some artifact
                    | None -> return None
                | None -> return None
        }

        member _.Store(inputs: IArtifact seq, cacheData: TaskCacheData, output: IArtifact): Task = task {
            match! computeCacheKey inputs cacheData with
            | None -> ()  // Non-cacheable, skip
            | Some cacheKeyHash ->
                let cacheDir = getCacheDirectory cacheKeyHash
                ensureCleanDirectory cacheDir
                do! cacheData.StoreTo cacheDir output
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
