namespace Meganob

open System
open System.IO
open System.Text.Json
open System.Threading.Tasks
open TruePath
open TruePath.SystemIo

type internal CacheEntry = {
    TypeDiscriminator: string
    Key: JsonElement
    LastAccessed: DateTimeOffset
}

type CleanupResult = {
    EntriesRemoved: int
    BytesFreed: int64
}

type internal CacheManager(config: CacheConfig) =
    let cacheMetadataFileName = "cache.json"

    let getHashedDirectory(key: ISerializableKey): AbsolutePath =
        let hash = CacheKey.ComputeHash key
        config.CacheFolder / hash

    let ensureDirectoryExists(path: AbsolutePath) =
        Directory.CreateDirectory(path.Value) |> ignore

    let readEntry(cacheDir: AbsolutePath) = task {
        let metadataPath = cacheDir / cacheMetadataFileName
        if metadataPath.ExistsFile() then
            try
                let! json = metadataPath.ReadAllTextAsync()
                let doc = JsonDocument.Parse(json)
                let root = doc.RootElement
                return Some {
                    TypeDiscriminator = root.GetProperty("TypeDiscriminator").GetString()
                    Key = root.GetProperty("Key").Clone()
                    LastAccessed = root.GetProperty("LastAccessed").GetDateTimeOffset()
                }
            with
            | _ -> return None
        else
            return None
    }

    let writeEntryJson(metadataPath: AbsolutePath, typeDiscriminator: string, keyJson: JsonElement, lastAccessed: DateTimeOffset): Task<unit> = task {
        use stream = new MemoryStream()
        use writer = new Utf8JsonWriter(stream, JsonWriterOptions(Indented = true))
        writer.WriteStartObject()
        writer.WriteString("TypeDiscriminator", typeDiscriminator)
        writer.WritePropertyName("Key")
        keyJson.WriteTo(writer)
        writer.WriteString("LastAccessed", lastAccessed)
        writer.WriteEndObject()
        do! writer.FlushAsync()

        let json = Text.Encoding.UTF8.GetString(stream.ToArray())
        do! metadataPath.WriteAllTextAsync json
    }

    let writeEntryInternal(cacheDir: AbsolutePath, key: ISerializableKey): Task<unit> = task {
        ensureDirectoryExists cacheDir
        let metadataPath = cacheDir / cacheMetadataFileName
        do! writeEntryJson(metadataPath, key.TypeDiscriminator, key.ToJson(), DateTimeOffset.UtcNow)
    }

    member _.GetCacheDirectory(key: ISerializableKey): AbsolutePath =
        let cacheDir = getHashedDirectory key
        ensureDirectoryExists cacheDir
        cacheDir

    member _.TouchEntry(key: ISerializableKey): Task<unit> = task {
        let cacheDir = getHashedDirectory key
        match! readEntry cacheDir with
        | Some entry ->
            let metadataPath = cacheDir / cacheMetadataFileName
            do! writeEntryJson(metadataPath, entry.TypeDiscriminator, entry.Key, DateTimeOffset.UtcNow)
        | None -> ()
    }

    member _.HasEntry(key: ISerializableKey): bool =
        let cacheDir = getHashedDirectory key
        let metadataPath = cacheDir / cacheMetadataFileName
        metadataPath.ExistsFile()

    member _.WriteEntry(key: ISerializableKey): Task<unit> =
        let cacheDir = getHashedDirectory key
        writeEntryInternal(cacheDir, key)

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
                        let dirInfo = DirectoryInfo(dir)
                        let size = dirInfo.EnumerateFiles("*", SearchOption.AllDirectories) |> Seq.sumBy _.Length

                        if verbose then
                            printfn $"  Removing: %s{hash}"

                        Directory.Delete(dir, true)
                        entriesRemoved <- entriesRemoved + 1
                        bytesFreed <- bytesFreed + size
                | None ->
                    failwithf $"Invalid metadata directory found: \"%s{dir}\". Please remove manually."

        return { EntriesRemoved = entriesRemoved; BytesFreed = bytesFreed }
    }
