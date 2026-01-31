namespace Meganob

open System.Collections.Generic
open System.Threading.Tasks
open TruePath
open TruePath.SystemIo

/// Function that loads an artifact from a cache directory
type ArtifactLoader = AbsolutePath -> Task<IArtifact option>

/// Registry for cacheable artifact types
type ArtifactTypeRegistry() =
    let loaders = Dictionary<string, ArtifactLoader>()

    /// Register an artifact type with its loader function
    member _.Register(typeName: string, loader: ArtifactLoader) =
        loaders[typeName] <- loader

    /// Try to get a loader for the given type name
    member _.TryGetLoader(typeName: string): ArtifactLoader option =
        match loaders.TryGetValue(typeName) with
        | true, loader -> Some loader
        | false, _ -> None

    /// Check if a type is registered
    member _.IsRegistered(typeName: string): bool =
        loaders.ContainsKey(typeName)

    /// Get the type name for an artifact (based on its .NET type)
    member _.GetTypeName(artifact: IArtifact): string option =
        let typeName = artifact.GetType().Name
        if loaders.ContainsKey(typeName) then Some typeName else None

module ArtifactTypeRegistry =
    /// Create a registry with built-in types pre-registered
    let createDefault(): ArtifactTypeRegistry =
        let registry = ArtifactTypeRegistry()

        // Register FileResult
        registry.Register("FileResult", fun cacheDir -> task {
            let files = System.IO.Directory.GetFiles(cacheDir.Value)
                        |> Array.filter (fun f -> System.IO.Path.GetFileName(f) <> "cache.json")
            if files.Length > 0 then
                return Some (FileResult(AbsolutePath(files[0])) :> IArtifact)
            else
                return None
        })

        // Register DirectoryResult
        registry.Register("DirectoryResult", fun cacheDir -> task {
            let dataDir = cacheDir / "data"
            if dataDir.ExistsDirectory() then
                return Some (DirectoryResult(dataDir) :> IArtifact)
            else
                return None
        })

        registry
