namespace Meganob

open System.Threading.Tasks
open TruePath

[<Interface>]
type IArtifact =
    abstract member GetContentHash: unit -> Task<Hash>

/// Cache configuration for a build task
type TaskCacheData = {
    /// Version string included in cache key (e.g., "build.v1")
    /// Different versions = different cache entries even with same inputs
    Version: string
    /// Store the task output to cache directory
    StoreTo: AbsolutePath -> IArtifact -> Task
    /// Load the task output from cache directory
    LoadFrom: AbsolutePath -> Task<IArtifact option>
}

[<Interface>]
type ICacheManager =
    /// Try to load cached output. Cache key computed from input hashes + cacheData.Version
    abstract member TryLoadCached: inputs: IArtifact seq * cacheData: TaskCacheData -> Task<IArtifact option>
    /// Store output to cache using cacheData.StoreTo
    abstract member Store: inputs: IArtifact seq * cacheData: TaskCacheData * output: IArtifact -> Task
