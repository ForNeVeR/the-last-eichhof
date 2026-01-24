module Meganob.Tests.DependencyTests

open System
open System.IO
open System.Text.Json
open System.Threading.Tasks
open Xunit
open TruePath
open Meganob

type TestKey(value: string) =
    interface ISerializableKey with
        member _.TypeDiscriminator = "TestKey"
        member _.ToJson() =
            let json = $"""{{ "value": "{value}" }}"""
            JsonDocument.Parse(json).RootElement.Clone()

let createMockContext() =
    let tempDir = Path.Combine(Path.GetTempPath(), $"meganob-dep-test-{Guid.NewGuid()}")
    Directory.CreateDirectory(tempDir) |> ignore
    let tempPath = AbsolutePath(tempDir)
    {
        new IDependencyContext with
            member _.Reporter = {
                new IReporter with
                    member _.Status _ = ()
                    member _.Log _ = ()
                    member _.WithProgress(_, _, action) = action ProgressReporter.Null
            }
            member _.TempFolder = tempPath
    }, tempPath

let cleanupDir (path: AbsolutePath) =
    if Directory.Exists(path.Value) then
        Directory.Delete(path.Value, true)

[<Fact>]
let ``NonCacheable dependency has no cache key`` () =
    let dep = Dependency.NonCacheable("test", fun _ -> Task.FromResult 42)

    Assert.True(dep.CacheKey.IsNone)

[<Fact>]
let ``NonCacheable dependency executes getter`` () = task {
    let context, tempDir = createMockContext()
    try
        let mutable executed = false
        let dep = Dependency.NonCacheable("test", fun _ -> task {
            executed <- true
            return 42
        })

        let! result = dep.Get(context)

        Assert.True(executed)
        Assert.Equal(42, result)
    finally
        cleanupDir tempDir
}

[<Fact>]
let ``Cacheable dependency has cache key`` () =
    let key = TestKey("test")
    let dep = Dependency.Cacheable(
        "test",
        key,
        (fun _ -> Task.FromResult 42),
        (fun _ _ -> task { () }),
        (fun _ -> Task.FromResult None)
    )

    Assert.True(dep.CacheKey.IsSome)
    Assert.Equal("TestKey", dep.CacheKey.Value.TypeDiscriminator)

[<Fact>]
let ``Cacheable dependency StoreToCache is called correctly`` () = task {
    let context, tempDir = createMockContext()
    let cacheDir = AbsolutePath(Path.Combine(Path.GetTempPath(), $"cache-test-{Guid.NewGuid()}"))
    Directory.CreateDirectory(cacheDir.Value) |> ignore
    try
        let mutable storedValue = 0
        let mutable storedDir: AbsolutePath option = None
        let key = TestKey("test")
        let dep = Dependency.Cacheable(
            "test",
            key,
            (fun _ -> Task.FromResult 42),
            (fun dir value -> task {
                storedDir <- Some dir
                storedValue <- value
            }),
            (fun _ -> Task.FromResult None)
        )

        let! result = (dep :> IDependency).GetForStore(context)
        do! (dep :> IDependency).StoreToCache(cacheDir, result)

        Assert.Equal(42, storedValue)
        Assert.Equal(cacheDir.Value, storedDir.Value.Value)
    finally
        cleanupDir tempDir
        cleanupDir cacheDir
}

[<Fact>]
let ``Cacheable dependency LoadFromCache returns cached value`` () = task {
    let cacheDir = AbsolutePath(Path.Combine(Path.GetTempPath(), $"cache-test-{Guid.NewGuid()}"))
    Directory.CreateDirectory(cacheDir.Value) |> ignore
    try
        let key = TestKey("test")
        let dep = Dependency.Cacheable(
            "test",
            key,
            (fun _ -> Task.FromResult 42),
            (fun _ _ -> task { () }),
            (fun _ -> Task.FromResult (Some 100))
        )

        let! result = (dep :> IDependency).LoadFromCache(cacheDir)

        Assert.True(result.IsSome)
        Assert.Equal(100, result.Value :?> int)
    finally
        cleanupDir cacheDir
}

[<Fact>]
let ``DownloadFile dependency has correct cache key type`` () =
    let uri = Uri("https://example.com/file.zip")
    let hash = Sha256 "abc123"
    let dep = Dependency.DownloadFile(uri, hash)

    Assert.True(dep.CacheKey.IsSome)
    Assert.Equal("Meganob.DownloadFileKey", dep.CacheKey.Value.TypeDiscriminator)
