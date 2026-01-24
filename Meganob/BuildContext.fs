namespace Meganob

open System.Collections.Concurrent
open Spectre.Console

type internal BuildContext(spectre: ProgressContext, cacheManager: CacheManager) =
    let resolvedDependencies = ConcurrentDictionary<IDependency, obj>()
    interface IBuildContext with
        member this.Reporter = SpectreReporter spectre
        member this.StoreDependency(dependency, context: IDependencyContext) = task {
            let addValue value =
                let success = resolvedDependencies.TryAdd(dependency, value)
                if not success then failwithf $"Dependency %A{dependency} has been calculated twice."

            match dependency.CacheKey with
            | Some key ->
                let cacheDir = cacheManager.GetCacheDirectory(key)
                let! cached = dependency.LoadFromCache(cacheDir)
                match cached with
                | Some value ->
                    do! cacheManager.TouchEntry(key)
                    context.Reporter.Log $"{dependency.DisplayName} will be reused from cache."
                    addValue value
                | None ->
                    let! result = dependency.GetForStore context
                    do! dependency.StoreToCache(cacheDir, result)
                    do! cacheManager.WriteEntry(key)
                    addValue result
            | None ->
                let! result = dependency.GetForStore context
                addValue result
        }
