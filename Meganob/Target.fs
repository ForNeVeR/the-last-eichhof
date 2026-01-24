namespace Meganob

open System.Collections.Immutable

type Target = {
    Name: string
    Dependencies: ImmutableArray<IDependency>
}

module Target =
    let OfDependencies(name: string, dependencies: IDependency seq): Target =
        {
            Name = name
            Dependencies = dependencies.ToImmutableArray()
        }
