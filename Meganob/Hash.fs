namespace Meganob

open System

type Hash =
    | Sha256 of string

    member this.GetBytes() =
        match this with
        | Sha256 s -> Convert.FromHexString(s)

    member this.ToHexString() =
        match this with
        | Sha256 s -> s.ToLowerInvariant()
