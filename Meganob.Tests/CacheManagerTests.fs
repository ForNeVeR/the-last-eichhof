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
    let registry = ArtifactTypeRegistry.createDefault()
    // Register TestArtifact for existing tests
    registry.Register("TestArtifact", fun cacheDir -> task {
        let filePath = cacheDir / "test.txt"
        if filePath.ExistsFile() then
            let! content = filePath.ReadAllTextAsync()
            return Some (TestArtifact(content, true) :> IArtifact)
        else
            return None
    })
    {
        CacheFolder = AbsolutePath(tempDir)
        MaxAge = TimeSpan.FromDays(15.0)
        ArtifactRegistry = registry
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

[<Fact>]
let ``FileResult can be stored and loaded from cache`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("file-input", false) :> IArtifact]

        // Create a temp file to use as FileResult
        let tempFile = AbsolutePath(Path.Combine(Path.GetTempPath(), $"test-file-{Guid.NewGuid()}.txt"))
        do! tempFile.WriteAllTextAsync "file content"

        try
            let output = FileResult(tempFile) :> IArtifact
            do! manager.Store(inputs, output)

            let! result = manager.TryLoadCached(inputs)

            Assert.True(result.IsSome)
            Assert.IsType<FileResult>(result.Value) |> ignore
        finally
            if tempFile.ExistsFile() then File.Delete(tempFile.Value)
    finally
        cleanup config
}

[<Fact>]
let ``DirectoryResult can be stored and loaded from cache`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("dir-input", false) :> IArtifact]

        // Create a temp directory with a file
        let tempDir = AbsolutePath(Path.Combine(Path.GetTempPath(), $"test-dir-{Guid.NewGuid()}"))
        Directory.CreateDirectory(tempDir.Value) |> ignore
        let testFile = tempDir / "testfile.txt"
        do! testFile.WriteAllTextAsync "directory content"

        try
            let output = DirectoryResult(tempDir) :> IArtifact
            do! manager.Store(inputs, output)

            let! result = manager.TryLoadCached(inputs)

            Assert.True(result.IsSome)
            Assert.IsType<DirectoryResult>(result.Value) |> ignore
        finally
            if tempDir.ExistsDirectory() then Directory.Delete(tempDir.Value, true)
    finally
        cleanup config
}

[<Fact>]
let ``Cache entries without ArtifactType are treated as cache miss`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("old-input", false) :> IArtifact]

        // Manually create an old-style cache entry without ArtifactType
        let! inputHash = CacheKey.ComputeHash [TestArtifact("old-input", false) :> IArtifact]
        let cacheDir = config.CacheFolder / inputHash
        Directory.CreateDirectory(cacheDir.Value) |> ignore
        let metadataPath = cacheDir / "cache.json"
        let oldStyleJson = """{"CacheKeyHash": "test", "LastAccessed": "2024-01-01T00:00:00Z"}"""
        do! metadataPath.WriteAllTextAsync oldStyleJson

        // Also create a test file so there's cached data
        let testFile = cacheDir / "test.txt"
        do! testFile.WriteAllTextAsync "cached content"

        let! result = manager.TryLoadCached(inputs)

        // Should be cache miss since ArtifactType is missing
        Assert.True(result.IsNone)
    finally
        cleanup config
}

// Custom artifact type for testing custom registration
type CustomArtifact(data: string) =
    member _.Data = data

    interface IArtifact with
        member _.GetContentHash() = Task.FromResult (CacheKey.ComputeCombinedHash [data])
        member _.CacheData = Some { new ICacheable with
            member _.StoreTo(cacheDir) = task {
                let filePath = cacheDir / "custom.dat"
                do! filePath.WriteAllTextAsync data
            }
            member _.LoadFrom(cacheDir) = task {
                let filePath = cacheDir / "custom.dat"
                if filePath.ExistsFile() then
                    let! content = filePath.ReadAllTextAsync()
                    return Some (CustomArtifact(content) :> IArtifact)
                else
                    return None
            }
        }

[<Fact>]
let ``Custom artifact types can be registered and cached`` () = task {
    let tempDir = Path.Combine(Path.GetTempPath(), $"meganob-test-{Guid.NewGuid()}")
    let registry = ArtifactTypeRegistry.createDefault()

    // Register custom artifact type
    registry.Register("CustomArtifact", fun cacheDir -> task {
        let filePath = cacheDir / "custom.dat"
        if filePath.ExistsFile() then
            let! content = filePath.ReadAllTextAsync()
            return Some (CustomArtifact(content) :> IArtifact)
        else
            return None
    })

    let config = {
        CacheFolder = AbsolutePath(tempDir)
        MaxAge = TimeSpan.FromDays(15.0)
        ArtifactRegistry = registry
    }
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("custom-input", false) :> IArtifact]
        let output = CustomArtifact("custom data") :> IArtifact

        do! manager.Store(inputs, output)
        let! result = manager.TryLoadCached(inputs)

        Assert.True(result.IsSome)
        Assert.IsType<CustomArtifact>(result.Value) |> ignore
        let loaded = result.Value :?> CustomArtifact
        Assert.Equal("custom data", loaded.Data)
    finally
        cleanup config
}
