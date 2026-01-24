module Meganob.Tests.CacheKeyTests

open System
open System.Text.Json
open Xunit
open Meganob

type TestKey(value: string) =
    interface ISerializableKey with
        member _.TypeDiscriminator = "TestKey"
        member _.ToJson() =
            let json = $"""{{ "value": "{value}" }}"""
            JsonDocument.Parse(json).RootElement.Clone()

[<Fact>]
let ``ComputeHash returns deterministic hash for same input`` () =
    let key1 = TestKey("test-value")
    let key2 = TestKey("test-value")

    let hash1 = CacheKey.ComputeHash(key1)
    let hash2 = CacheKey.ComputeHash(key2)

    Assert.Equal(hash1, hash2)

[<Fact>]
let ``ComputeHash returns different hash for different values`` () =
    let key1 = TestKey("value1")
    let key2 = TestKey("value2")

    let hash1 = CacheKey.ComputeHash(key1)
    let hash2 = CacheKey.ComputeHash(key2)

    Assert.NotEqual<string>(hash1, hash2)

[<Fact>]
let ``ComputeHash returns valid hex string`` () =
    let key = TestKey("test")
    let hash = CacheKey.ComputeHash(key)

    Assert.Equal(64, hash.Length) // SHA256 produces 64 hex characters
    Assert.True(hash |> Seq.forall (fun c -> Char.IsAsciiHexDigitLower c))

[<Fact>]
let ``DownloadFileKey produces consistent hash`` () =
    let uri = Uri("https://example.com/file.zip")
    let hash = Sha256 "abc123"

    let key1 = DownloadFileKey(uri, hash) :> ISerializableKey
    let key2 = DownloadFileKey(uri, hash) :> ISerializableKey

    let hash1 = CacheKey.ComputeHash(key1)
    let hash2 = CacheKey.ComputeHash(key2)

    Assert.Equal(hash1, hash2)

[<Fact>]
let ``DownloadFileKey has correct type discriminator`` () =
    let uri = Uri("https://example.com/file.zip")
    let hash = Sha256 "abc123"
    let key = DownloadFileKey(uri, hash) :> ISerializableKey

    Assert.Equal("Meganob.DownloadFileKey", key.TypeDiscriminator)
