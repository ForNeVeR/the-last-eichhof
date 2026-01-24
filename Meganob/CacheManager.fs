namespace Meganob

open System
open System.IO
open System.Text.Json
open System.Threading.Tasks
open TruePath

type internal CacheEntry = {
    TypeDiscriminator: string
    Key: JsonElement
    LastAccessed: DateTimeOffset
}

type internal CacheManager(config: CacheConfig) =
    let cacheMetadataFileName = "cache.json"

    let getHashedDirectory(key: ISerializableKey): AbsolutePath =
        let hash = CacheKey.ComputeHash(key)
        config.CacheFolder / hash

    let ensureDirectoryExists(path: AbsolutePath) =
        Directory.CreateDirectory(path.Value) |> ignore

    let readEntry(cacheDir: AbsolutePath): CacheEntry option =
        let metadataPath = cacheDir / cacheMetadataFileName
        if File.Exists(metadataPath.Value) then
            try
                let json = File.ReadAllText(metadataPath.Value)
                let doc = JsonDocument.Parse(json)
                let root = doc.RootElement
                Some {
                    TypeDiscriminator = root.GetProperty("TypeDiscriminator").GetString()
                    Key = root.GetProperty("Key").Clone()
                    LastAccessed = root.GetProperty("LastAccessed").GetDateTimeOffset()
                }
            with
            | _ -> None
        else
            None

    let writeEntryJson(metadataPath: AbsolutePath, typeDiscriminator: string, keyJson: JsonElement, lastAccessed: DateTimeOffset): Task<unit> = task {
        use stream = new MemoryStream()
        use writer = new Utf8JsonWriter(stream, JsonWriterOptions(Indented = true))
        writer.WriteStartObject()
        writer.WriteString("TypeDiscriminator", typeDiscriminator)
        writer.WritePropertyName("Key")
        keyJson.WriteTo(writer)
        writer.WriteString("LastAccessed", lastAccessed)
        writer.WriteEndObject()
        writer.Flush()

        let json = System.Text.Encoding.UTF8.GetString(stream.ToArray())
        do! File.WriteAllTextAsync(metadataPath.Value, json)
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
        match readEntry cacheDir with
        | Some entry ->
            let metadataPath = cacheDir / cacheMetadataFileName
            do! writeEntryJson(metadataPath, entry.TypeDiscriminator, entry.Key, DateTimeOffset.UtcNow)
        | None -> ()
    }

    member _.HasEntry(key: ISerializableKey): bool =
        let cacheDir = getHashedDirectory key
        let metadataPath = cacheDir / cacheMetadataFileName
        File.Exists(metadataPath.Value)

    member _.WriteEntry(key: ISerializableKey): Task<unit> =
        let cacheDir = getHashedDirectory key
        writeEntryInternal(cacheDir, key)

    member _.Cleanup(): Task<unit> = task {
        if Directory.Exists(config.CacheFolder.Value) then
            let now = DateTimeOffset.UtcNow
            let directories = Directory.GetDirectories(config.CacheFolder.Value)
            for dir in directories do
                let cacheDir = AbsolutePath dir
                match readEntry cacheDir with
                | Some entry ->
                    let age = now - entry.LastAccessed
                    if age > config.MaxAge then
                        Directory.Delete(dir, true)
                | None ->
                    // No valid metadata, delete the directory
                    Directory.Delete(dir, true)
    }
