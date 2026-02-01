// SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
//
// SPDX-License-Identifier: MIT

namespace Meganob

open System
open System.Collections.Immutable
open System.IO
open System.Security.Cryptography
open System.Text
open System.Threading.Tasks

module CacheKey =
    let ComputeHash (artifacts: IArtifact seq): Task<string> = task {
        use sha256 = SHA256.Create()
        use ms = new MemoryStream()
        for artifact in artifacts do
            let! hash = artifact.GetContentHash()
            ms.Write(hash.GetBytes())
        ms.Position <- 0L
        let hashBytes = sha256.ComputeHash(ms)
        return Convert.ToHexStringLower hashBytes
    }

    let ComputeCombinedHash (values: string seq): Hash =
        use sha256 = SHA256.Create()
        let combined = String.Join("|", values)
        let bytes = Encoding.UTF8.GetBytes(combined)
        let hashBytes = sha256.ComputeHash(bytes)
        Sha256 (Convert.ToHexString(hashBytes).ToLowerInvariant())

/// A build task with dependencies (inputs) that must complete before execution.
/// The Inputs array uses ImmutableArray which captures references at creation time.
/// This makes dependency cycles structurally impossible - when a task is created,
/// all its dependencies must already exist as fully-formed objects, so they cannot reference it.
[<ReferenceEquality>]
type BuildTask =
    {
        Name: string
        Inputs: ImmutableArray<BuildTask>
        CacheData: TaskCacheData option  // None = non-cacheable
        Execute: BuildContext * IArtifact seq -> Task<IArtifact>
    }
