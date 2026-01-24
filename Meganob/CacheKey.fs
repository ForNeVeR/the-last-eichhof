namespace Meganob

open System
open System.Security.Cryptography
open System.Text.Json

[<Interface>]
type ISerializableKey =
    abstract member TypeDiscriminator: string
    abstract member ToJson: unit -> JsonElement

module CacheKey =
    let ComputeHash(key: ISerializableKey): string =
        use doc = JsonDocument.Parse("{}")
        use stream = new System.IO.MemoryStream()
        use writer = new Utf8JsonWriter(stream)
        writer.WriteStartObject()
        writer.WriteString("type", key.TypeDiscriminator)
        writer.WritePropertyName("key")
        key.ToJson().WriteTo(writer)
        writer.WriteEndObject()
        writer.Flush()

        let bytes = stream.ToArray()
        let hashBytes = SHA256.HashData(bytes)
        Convert.ToHexStringLower(hashBytes)
