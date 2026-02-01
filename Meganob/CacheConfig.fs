// SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
//
// SPDX-License-Identifier: MIT

namespace Meganob

open System
open TruePath

type CacheConfig = {
    CacheFolder: AbsolutePath
    MaxAge: TimeSpan
}

module CacheConfig =
    let Default = {
        CacheFolder =
            match Option.ofObj <| Environment.GetEnvironmentVariable "MEGANOB_CACHE_BASE" with
            | Some cacheBase -> AbsolutePath cacheBase
            | None -> AbsolutePath(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData))
                      / "Meganob" / "cache"
        MaxAge = TimeSpan.FromDays 15.0
    }
