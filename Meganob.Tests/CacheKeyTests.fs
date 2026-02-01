// SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
//
// SPDX-License-Identifier: MIT

module Meganob.Tests.CacheKeyTests

open System.Threading.Tasks
open Xunit
open Meganob

type TestArtifact(value: string) =
    interface IArtifact with
        member _.GetContentHash() = Task.FromResult <| CacheKey.ComputeCombinedHash [value]

[<Fact>]
let ``ComputeHash returns deterministic hash for same input``(): Task = task {
    let artifact1 = TestArtifact("test-value")
    let artifact2 = TestArtifact("test-value")

    let! hash1 = CacheKey.ComputeHash([artifact1])
    let! hash2 = CacheKey.ComputeHash([artifact2])

    Assert.Equal(hash1, hash2)
}

[<Fact>]
let ``ComputeHash returns different hash for different values``(): Task = task {
    let artifact1 = TestArtifact("value1")
    let artifact2 = TestArtifact("value2")

    let! hash1 = CacheKey.ComputeHash([artifact1])
    let! hash2 = CacheKey.ComputeHash([artifact2])

    Assert.NotEqual<string>(hash1, hash2)
}

[<Fact>]
let ``ComputeHash returns valid hex string``(): Task = task {
    let artifact = TestArtifact("test")
    let! hash = CacheKey.ComputeHash([artifact])

    Assert.Equal(64, hash.Length) // SHA256 produces 64 hex characters
    Assert.True(hash |> Seq.forall System.Char.IsAsciiHexDigitLower)
}

[<Fact>]
let ``ComputeCombinedHash produces consistent hash`` () =
    let hash1 = CacheKey.ComputeCombinedHash ["https://example.com/file.zip"; "abc123"]
    let hash2 = CacheKey.ComputeCombinedHash ["https://example.com/file.zip"; "abc123"]

    Assert.Equal(hash1, hash2)

[<Fact>]
let ``ComputeCombinedHash produces different hash for different values`` () =
    let hash1 = CacheKey.ComputeCombinedHash ["https://example.com/file1.zip"; "abc123"]
    let hash2 = CacheKey.ComputeCombinedHash ["https://example.com/file2.zip"; "abc123"]

    Assert.NotEqual(hash1, hash2)
