namespace Meganob

open TruePath

type DirectoryResult(path: AbsolutePath) =
    member _.Path = path
    interface ITargetResult

type FileResult(path: AbsolutePath) =
    member _.Path = path
    interface ITargetResult
