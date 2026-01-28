namespace Meganob

open System.Threading.Tasks
open TruePath

/// For artifacts that can be stored/loaded from cache directory
[<Interface>]
type ICacheable =
    abstract member StoreTo: cacheDir: AbsolutePath -> Task
    abstract member LoadFrom: cacheDir: AbsolutePath -> Task<IArtifact option>

and [<Interface>] IArtifact =
    abstract member GetContentHash: unit -> Task<Hash>
    abstract member CacheData: ICacheable option  // None = hash-only, not storable

[<Interface>]
type ICacheManager =
    /// Try to load cached output. Cache key computed from input hashes.
    abstract member TryLoadCached: inputs: IArtifact seq -> Task<IArtifact option>
    /// Store output to cache. Only stores if output.CacheData.IsSome.
    abstract member Store: inputs: IArtifact seq * output: IArtifact -> Task
