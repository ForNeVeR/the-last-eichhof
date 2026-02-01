module Meganob.Tests.CacheManagerTests

open System
open System.IO
open System.Threading.Tasks
open Xunit
open TruePath
open TruePath.SystemIo
open Meganob

type TestArtifact(value: string) =
    let hash = CacheKey.ComputeCombinedHash [value]
    member _.Value = value

    interface IArtifact with
        member _.GetContentHash() = Task.FromResult hash

module TestArtifact =
    let CacheData(version: string): TaskCacheData = {
        Version = version
        StoreTo = fun cacheDir output ->
            let artifact = output :?> TestArtifact
            let filePath = cacheDir / "test.txt"
            filePath.WriteAllTextAsync artifact.Value
        LoadFrom = fun cacheDir -> task {
            let filePath = cacheDir / "test.txt"
            if filePath.ExistsFile() then
                let! content = filePath.ReadAllTextAsync()
                return Some (TestArtifact(content) :> IArtifact)
            else
                return None
        }
    }

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
        let inputs = [TestArtifact("input") :> IArtifact]
        let cacheData = TestArtifact.CacheData "test.v1"

        let! result = manager.TryLoadCached(inputs, cacheData)

        Assert.True(result.IsNone)
    finally
        cleanup config
}

[<Fact>]
let ``Store and TryLoadCached work together`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("input") :> IArtifact]
        let output = TestArtifact("output-value") :> IArtifact
        let cacheData = TestArtifact.CacheData "test.v1"

        do! manager.Store(inputs, cacheData, output)
        let! result = manager.TryLoadCached(inputs, cacheData)

        Assert.True(result.IsSome)
    finally
        cleanup config
}

[<Fact>]
let ``Different versions create different cache entries`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("input") :> IArtifact]
        let output = TestArtifact("output-value") :> IArtifact
        let cacheDataV1 = TestArtifact.CacheData "test.v1"
        let cacheDataV2 = TestArtifact.CacheData "test.v2"

        // Store with v1
        do! manager.Store(inputs, cacheDataV1, output)

        // Load with v2 should be cache miss
        let! resultV2 = manager.TryLoadCached(inputs, cacheDataV2)
        Assert.True(resultV2.IsNone)

        // Load with v1 should be cache hit
        let! resultV1 = manager.TryLoadCached(inputs, cacheDataV1)
        Assert.True(resultV1.IsSome)
    finally
        cleanup config
}

[<Fact>]
let ``TryLoadCached returns None for empty inputs`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let cacheData = TestArtifact.CacheData "test.v1"

        let! result = manager.TryLoadCached([], cacheData)

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
        let inputs = [TestArtifact("input") :> IArtifact]
        let output = TestArtifact("output-value") :> IArtifact
        let cacheData = TestArtifact.CacheData "test.v1"

        do! managerInterface.Store(inputs, cacheData, output)

        // Small delay to ensure entry is "old" (MaxAge is zero)
        do! Task.Delay(10)

        // Run cleanup
        let! result = manager.Cleanup(false)

        // Verify entry was removed
        Assert.Equal(1, result.EntriesRemoved)
        Assert.True(result.BytesFreed > 0L)

        // Verify cache is empty
        let! cached = managerInterface.TryLoadCached(inputs, cacheData)
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
        let inputs = [TestArtifact("input") :> IArtifact]
        let output = TestArtifact("output-value") :> IArtifact
        let cacheData = TestArtifact.CacheData "test.v1"

        do! managerInterface.Store(inputs, cacheData, output)

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
        let inputs = [TestArtifact("file-input") :> IArtifact]
        let cacheData = FileResult.CacheData "file.v1"

        // Create a temp file to use as FileResult
        let tempFile = AbsolutePath(Path.Combine(Path.GetTempPath(), $"test-file-{Guid.NewGuid()}.txt"))
        do! tempFile.WriteAllTextAsync "file content"

        try
            let output = FileResult(tempFile) :> IArtifact
            do! manager.Store(inputs, cacheData, output)

            let! result = manager.TryLoadCached(inputs, cacheData)

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
        let inputs = [TestArtifact("dir-input") :> IArtifact]
        let cacheData = DirectoryResult.CacheData "dir.v1"

        // Create a temp directory with a file
        let tempDir = AbsolutePath(Path.Combine(Path.GetTempPath(), $"test-dir-{Guid.NewGuid()}"))
        Directory.CreateDirectory(tempDir.Value) |> ignore
        let testFile = tempDir / "testfile.txt"
        do! testFile.WriteAllTextAsync "directory content"

        try
            let output = DirectoryResult(tempDir) :> IArtifact
            do! manager.Store(inputs, cacheData, output)

            let! result = manager.TryLoadCached(inputs, cacheData)

            Assert.True(result.IsSome)
            Assert.IsType<DirectoryResult>(result.Value) |> ignore
        finally
            if tempDir.ExistsDirectory() then Directory.Delete(tempDir.Value, true)
    finally
        cleanup config
}

// Custom artifact type for testing custom cache data
type CustomArtifact(data: string) =
    member _.Data = data

    interface IArtifact with
        member _.GetContentHash() = Task.FromResult (CacheKey.ComputeCombinedHash [data])

module CustomArtifact =
    let CacheData(version: string): TaskCacheData = {
        Version = version
        StoreTo = fun cacheDir output ->
            let artifact = output :?> CustomArtifact
            let filePath = cacheDir / "custom.dat"
            filePath.WriteAllTextAsync artifact.Data
        LoadFrom = fun cacheDir -> task {
            let filePath = cacheDir / "custom.dat"
            if filePath.ExistsFile() then
                let! content = filePath.ReadAllTextAsync()
                return Some (CustomArtifact(content) :> IArtifact)
            else
                return None
        }
    }

[<Fact>]
let ``DirectoryResult preserves nested subdirectory structure in cache`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("nested-dir-input") :> IArtifact]
        let cacheData = DirectoryResult.CacheData "dir.v1"

        // Create a temp directory with nested structure
        let tempDir = AbsolutePath(Path.Combine(Path.GetTempPath(), $"test-nested-dir-{Guid.NewGuid()}"))
        Directory.CreateDirectory(tempDir.Value) |> ignore

        // Create root/file1.txt
        let file1 = tempDir / "file1.txt"
        do! file1.WriteAllTextAsync "content1"

        // Create root/sub/file2.txt
        let subDir = tempDir / "sub"
        Directory.CreateDirectory(subDir.Value) |> ignore
        let file2 = subDir / "file2.txt"
        do! file2.WriteAllTextAsync "content2"

        // Create root/sub/deep/file3.txt
        let deepDir = subDir / "deep"
        Directory.CreateDirectory(deepDir.Value) |> ignore
        let file3 = deepDir / "file3.txt"
        do! file3.WriteAllTextAsync "content3"

        try
            let output = DirectoryResult(tempDir) :> IArtifact
            do! manager.Store(inputs, cacheData, output)

            let! result = manager.TryLoadCached(inputs, cacheData)

            Assert.True(result.IsSome)
            let loadedDir = (result.Value :?> DirectoryResult).Path

            // Verify all files exist with correct relative paths
            let loadedFile1 = loadedDir / "file1.txt"
            let loadedFile2 = loadedDir / "sub" / "file2.txt"
            let loadedFile3 = loadedDir / "sub" / "deep" / "file3.txt"

            Assert.True(loadedFile1.ExistsFile(), "file1.txt should exist")
            Assert.True(loadedFile2.ExistsFile(), "sub/file2.txt should exist")
            Assert.True(loadedFile3.ExistsFile(), "sub/deep/file3.txt should exist")

            // Verify content is preserved
            let! content1 = loadedFile1.ReadAllTextAsync()
            let! content2 = loadedFile2.ReadAllTextAsync()
            let! content3 = loadedFile3.ReadAllTextAsync()

            Assert.Equal("content1", content1)
            Assert.Equal("content2", content2)
            Assert.Equal("content3", content3)
        finally
            if tempDir.ExistsDirectory() then Directory.Delete(tempDir.Value, true)
    finally
        cleanup config
}

[<Fact>]
let ``Custom artifact types can be cached`` () = task {
    let config = createTempCacheConfig()
    try
        let manager = CacheManager(config) :> ICacheManager
        let inputs = [TestArtifact("custom-input") :> IArtifact]
        let output = CustomArtifact("custom data") :> IArtifact
        let cacheData = CustomArtifact.CacheData "custom.v1"

        do! manager.Store(inputs, cacheData, output)
        let! result = manager.TryLoadCached(inputs, cacheData)

        Assert.True(result.IsSome)
        Assert.IsType<CustomArtifact>(result.Value) |> ignore
        let loaded = result.Value :?> CustomArtifact
        Assert.Equal("custom data", loaded.Data)
    finally
        cleanup config
}
