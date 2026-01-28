module Meganob.Tests.CacheManagerTests

open System
open System.IO
open System.Threading.Tasks
open Xunit
open TruePath
open TruePath.SystemIo
open Meganob

type TestArtifact(value: string, cacheable: bool) =
    let hash = CacheKey.ComputeCombinedHash [value]

    interface IArtifact with
        member _.GetContentHash() = Task.FromResult hash
        member _.CacheData =
            if cacheable then
                Some { new ICacheable with
                    member _.StoreTo(cacheDir) = task {
                        let filePath = cacheDir / "test.txt"
                        do! filePath.WriteAllTextAsync value
                    }
                    member _.LoadFrom(cacheDir) = task {
                        let filePath = cacheDir / "test.txt"
                        if filePath.ExistsFile() then
                            let! content = filePath.ReadAllTextAsync()
                            return Some (TestArtifact(content, true) :> IArtifact)
                        else
                            return None
                    }
                }
            else
                None

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
let ``TryLoadCached returns None when cache is empty`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("input", false) :> IArtifact]

        let! result = manager.TryLoadCached(inputs)

        Assert.True(result.IsNone)
    finally
        cleanup config
}

[<Fact>]
let ``Store and TryLoadCached work together`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("input", false) :> IArtifact]
        let output = TestArtifact("output-value", true) :> IArtifact

        do! manager.Store(inputs, output)
        let! result = manager.TryLoadCached(inputs)

        Assert.True(result.IsSome)
    finally
        cleanup config
}

[<Fact>]
let ``Store does nothing for non-cacheable outputs`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("input", false) :> IArtifact]
        let output = TestArtifact("output-value", false) :> IArtifact  // Not cacheable

        do! manager.Store(inputs, output)
        let! result = manager.TryLoadCached(inputs)

        Assert.True(result.IsNone)  // Nothing was stored
    finally
        cleanup config
}

[<Fact>]
let ``TryLoadCached returns None for empty inputs`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager

        let! result = manager.TryLoadCached([])

        Assert.True(result.IsNone)  // No inputs = non-cacheable
    finally
        cleanup config
}

[<Fact>]
let ``Cleanup removes old entries`` () = task {
    let config = { createTempCacheConfig() with MaxAge = TimeSpan.Zero }
    try
        let manager = CacheManager(config)
        let managerInterface = manager :> ICacheManager
        let inputs = [TestArtifact("input", false) :> IArtifact]
        let output = TestArtifact("output-value", true) :> IArtifact

        do! managerInterface.Store(inputs, output)

        // Small delay to ensure entry is "old" (MaxAge is zero)
        do! Task.Delay(10)

        // Run cleanup
        let! result = manager.Cleanup(false)

        // Verify entry was removed
        Assert.Equal(1, result.EntriesRemoved)
        Assert.True(result.BytesFreed > 0L)

        // Verify cache is empty
        let! cached = managerInterface.TryLoadCached(inputs)
        Assert.True(cached.IsNone)
    finally
        cleanup config
}

[<Fact>]
let ``Cleanup keeps recent entries`` () = task {
    let config = { createTempCacheConfig() with MaxAge = TimeSpan.FromDays(1.0) }
    try
        let manager = CacheManager(config)
        let managerInterface = manager :> ICacheManager
        let inputs = [TestArtifact("input", false) :> IArtifact]
        let output = TestArtifact("output-value", true) :> IArtifact

        do! managerInterface.Store(inputs, output)

        // Run cleanup
        let! result = manager.Cleanup(false)

        // Verify entry still exists (it's recent)
        Assert.Equal(0, result.EntriesRemoved)
        Assert.Equal(0L, result.BytesFreed)
    finally
        cleanup config
}

[<Fact>]
let ``Cleanup handles empty cache folder gracefully`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config)

        // Should not throw even if cache folder doesn't exist
        let! result = manager.Cleanup(false)

        Assert.Equal(0, result.EntriesRemoved)
        Assert.Equal(0L, result.BytesFreed)
    finally
        cleanup config
}
