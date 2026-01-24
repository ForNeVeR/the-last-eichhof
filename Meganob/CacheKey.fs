namespace Meganob

open System
open System.Security.Cryptography
open System.Text
open System.Text.Json

[<Interface>]
type ISerializableKey =
    abstract member TypeDiscriminator: string
    abstract member ToJson: unit -> JsonElement

module CacheKey =
    let ComputeHash(key: ISerializableKey): string =
        // Key's hash should depend on its type discriminator and its content, nothing else.
        // Discriminator is needed for a case when several keys have the same representation, e.g. dummy "{}".
        use stream = new System.IO.MemoryStream()
        do
            use writer = new Utf8JsonWriter(stream)
            key.ToJson().WriteTo(writer)

        stream.Write(Encoding.UTF8.GetBytes key.TypeDiscriminator)

        let bytes = stream.ToArray()
        let hashBytes = SHA256.HashData(bytes)
        Convert.ToHexStringLower(hashBytes)
