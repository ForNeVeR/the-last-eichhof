namespace Meganob

open System.Collections.Concurrent
open Spectre.Console

type internal BuildContext(spectre: ProgressContext, ?cacheManager: CacheManager) =
    let resolvedDependencies = ConcurrentDictionary<IDependency, obj>()
    interface IBuildContext with
        member this.Reporter = SpectreReporter spectre
        member this.StoreDependency(dependency, context: IDependencyContext) = task {
            match dependency.CacheKey, cacheManager with
            | Some key, Some cm ->
                let cacheDir = cm.GetCacheDirectory(key)
                let! cached = dependency.LoadFromCache(cacheDir)
                match cached with
                | Some value ->
                    do! cm.TouchEntry(key)
                    context.Reporter.Log $"{dependency.DisplayName} will be reused from cache."
                    let success = resolvedDependencies.TryAdd(dependency, value)
                    if not success then failwithf $"Dependency %A{dependency} has been calculated twice."
                | None ->
                    let! result = dependency.GetForStore context
                    do! dependency.StoreToCache(cacheDir, result)
                    do! cm.WriteEntry(key)
                    let success = resolvedDependencies.TryAdd(dependency, result)
                    if not success then failwithf $"Dependency %A{dependency} has been calculated twice."
            | _ ->
                let! result = dependency.GetForStore context
                let success = resolvedDependencies.TryAdd(dependency, result)
                if not success then failwithf $"Dependency %A{dependency} has been calculated twice."
        }
