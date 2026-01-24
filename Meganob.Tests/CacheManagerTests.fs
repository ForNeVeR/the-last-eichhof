module Meganob.Tests.CacheManagerTests

open System
open System.IO
open System.Text.Json
open Xunit
open TruePath
open Meganob

type TestKey(value: string) =
    interface ISerializableKey with
        member _.TypeDiscriminator = "TestKey"
        member _.ToJson() =
            let json = $"""{{ "value": "{value}" }}"""
            JsonDocument.Parse(json).RootElement.Clone()

let createTempCacheConfig () =
    let tempDir = Path.Combine(Path.GetTempPath(), $"meganob-test-{Guid.NewGuid()}")
    {
        CacheFolder = AbsolutePath(tempDir)
        MaxAge = TimeSpan.FromDays(15.0)
    }

let cleanup (config: CacheConfig) =
    if Directory.Exists(config.CacheFolder.Value) then
        Directory.Delete(config.CacheFolder.Value, true)

[<Fact>]
let ``GetCacheDirectory creates directory if not exists`` () =
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config)
        let key = TestKey("test")

        let cacheDir = manager.GetCacheDirectory(key)

        Assert.True(Directory.Exists(cacheDir.Value))
    finally
        cleanup config

[<Fact>]
let ``GetCacheDirectory returns consistent path for same key`` () =
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config)
        let key = TestKey("test")

        let cacheDir1 = manager.GetCacheDirectory(key)
        let cacheDir2 = manager.GetCacheDirectory(key)

        Assert.Equal(cacheDir1.Value, cacheDir2.Value)
    finally
        cleanup config

[<Fact>]
let ``HasEntry returns false when entry does not exist`` () =
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config)
        let key = TestKey("nonexistent")

        let exists = manager.HasEntry(key)

        Assert.False(exists)
    finally
        cleanup config

[<Fact>]
let ``WriteEntry and HasEntry work together`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config)
        let key = TestKey("test")

        do! manager.WriteEntry(key)
        let exists = manager.HasEntry(key)

        Assert.True(exists)
    finally
        cleanup config
}

[<Fact>]
let ``TouchEntry updates last access time`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config)
        let key = TestKey("test")

        do! manager.WriteEntry(key)

        // Small delay to ensure time difference
        do! System.Threading.Tasks.Task.Delay(50)

        do! manager.TouchEntry(key)

        // Read cache.json to verify last access time was updated
        let cacheDir = manager.GetCacheDirectory(key)
        let metadataPath = cacheDir / "cache.json"
        let json = File.ReadAllText(metadataPath.Value)
        let doc = JsonDocument.Parse(json)
        let lastAccessed = doc.RootElement.GetProperty("LastAccessed").GetDateTimeOffset()

        // Last accessed should be very recent
        let timeSinceAccess = DateTimeOffset.UtcNow - lastAccessed
        Assert.True(timeSinceAccess.TotalSeconds < 5.0)
    finally
        cleanup config
}

[<Fact>]
let ``Cleanup removes old entries`` () = task {
    let config = { createTempCacheConfig() with MaxAge = TimeSpan.Zero }
    try
        let manager = CacheManager(config)
        let key = TestKey("old-entry")

        do! manager.WriteEntry(key)
        let cacheDir = manager.GetCacheDirectory(key)

        // Verify entry exists
        Assert.True(Directory.Exists(cacheDir.Value))

        // Small delay to ensure entry is "old" (MaxAge is zero)
        do! System.Threading.Tasks.Task.Delay(10)

        // Run cleanup
        do! manager.Cleanup()

        // Verify entry was removed
        Assert.False(Directory.Exists(cacheDir.Value))
    finally
        cleanup config
}

[<Fact>]
let ``Cleanup keeps recent entries`` () = task {
    let config = { createTempCacheConfig() with MaxAge = TimeSpan.FromDays(1.0) }
    try
        let manager = CacheManager(config)
        let key = TestKey("recent-entry")

        do! manager.WriteEntry(key)
        let cacheDir = manager.GetCacheDirectory(key)

        // Run cleanup
        do! manager.Cleanup()

        // Verify entry still exists (it's recent)
        Assert.True(Directory.Exists(cacheDir.Value))
    finally
        cleanup config
}

[<Fact>]
let ``Cleanup handles empty cache folder gracefully`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config)

        // Should not throw even if cache folder doesn't exist
        do! manager.Cleanup()

        Assert.True(true)
    finally
        cleanup config
}
