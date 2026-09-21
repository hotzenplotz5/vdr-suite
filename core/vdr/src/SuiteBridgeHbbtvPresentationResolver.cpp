#include "SuiteBridgeHbbtvPresentationResolver.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace
{

constexpr std::uint64_t MaximumEncodedBytes = 16U * 1024U * 1024U;
constexpr std::uint32_t MaximumRenderWidth = 3840U;
constexpr std::uint32_t MaximumRenderHeight = 2160U;
constexpr std::uint32_t PresentationChunkBytes = 49152U;

bool stringField(
    const std::string& json,
    const std::string& key,
    std::string& value,
    bool required = true)
{
    const std::string marker = "\"" + key + "\":\"";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos)
        return !required;

    std::size_t position = start + marker.size();
    value.clear();
    while (position < json.size())
    {
        const char character = json[position++];
        if (character == '"') return true;
        if (character == '\\')
        {
            if (position >= json.size()) return false;
            const char escaped = json[position++];
            if (escaped == '"' || escaped == '\\' || escaped == '/')
                value.push_back(escaped);
            else
                return false;
        }
        else if (static_cast<unsigned char>(character) < 0x20U)
        {
            return false;
        }
        else
        {
            value.push_back(character);
        }
    }
    return false;
}

bool unsignedField(
    const std::string& json,
    const std::string& key,
    std::uint64_t& value)
{
    const std::string marker = "\"" + key + "\":";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos) return false;

    std::size_t position = start + marker.size();
    while (position < json.size() &&
           std::isspace(static_cast<unsigned char>(json[position])))
        ++position;

    if (position >= json.size() ||
        !std::isdigit(static_cast<unsigned char>(json[position])))
        return false;

    std::uint64_t parsed = 0;
    while (position < json.size() &&
           std::isdigit(static_cast<unsigned char>(json[position])))
    {
        const unsigned int digit =
            static_cast<unsigned int>(json[position] - '0');
        if (parsed >
            (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
            return false;
        parsed = parsed * 10U + digit;
        ++position;
    }
    value = parsed;
    return true;
}

int base64Value(unsigned char character)
{
    if (character >= 'A' && character <= 'Z')
        return character - 'A';
    if (character >= 'a' && character <= 'z')
        return character - 'a' + 26;
    if (character >= '0' && character <= '9')
        return character - '0' + 52;
    if (character == '+') return 62;
    if (character == '/') return 63;
    return -1;
}

bool decodeBase64(
    const std::string& encoded,
    std::string& decoded)
{
    decoded.clear();
    if (encoded.empty() || encoded.size() % 4U != 0)
        return false;

    decoded.reserve((encoded.size() / 4U) * 3U);
    for (std::size_t offset = 0; offset < encoded.size(); offset += 4U)
    {
        const bool pad2 = encoded[offset + 2U] == '=';
        const bool pad3 = encoded[offset + 3U] == '=';
        if (pad2 && !pad3) return false;
        if ((pad2 || pad3) && offset + 4U != encoded.size())
            return false;

        const int a = base64Value(
            static_cast<unsigned char>(encoded[offset]));
        const int b = base64Value(
            static_cast<unsigned char>(encoded[offset + 1U]));
        const int c = pad2 ? 0 : base64Value(
            static_cast<unsigned char>(encoded[offset + 2U]));
        const int d = pad3 ? 0 : base64Value(
            static_cast<unsigned char>(encoded[offset + 3U]));
        if (a < 0 || b < 0 || c < 0 || d < 0)
            return false;

        const std::uint32_t value =
            (static_cast<std::uint32_t>(a) << 18U) |
            (static_cast<std::uint32_t>(b) << 12U) |
            (static_cast<std::uint32_t>(c) << 6U) |
            static_cast<std::uint32_t>(d);

        decoded.push_back(static_cast<char>((value >> 16U) & 0xffU));
        if (!pad2)
            decoded.push_back(static_cast<char>((value >> 8U) & 0xffU));
        if (!pad3)
            decoded.push_back(static_cast<char>(value & 0xffU));
    }
    return true;
}

struct Wire
{
    std::string operation;
    std::string result;
    std::uint64_t resultCode = 0;
    std::string sessionId;
    std::uint64_t frameRevision = 0;
    std::uint64_t observedAt = 0;
    std::uint64_t renderWidth = 0;
    std::uint64_t renderHeight = 0;
    std::uint64_t encodedBytes = 0;
    std::uint64_t returnedBytes = 0;
    std::string dataBase64;
};

bool parseWire(
    const SuiteBridgeHbbtvCommandReply& reply,
    const std::string& expectedOperation,
    const std::string& expectedSession,
    Wire& wire)
{
    if (!reply.transportSucceeded ||
        reply.replyCode != 250 ||
        reply.payload.size() > 80U * 1024U)
        return false;

    std::uint64_t schemaVersion = 0;
    std::uint64_t providerSchemaVersion = 0;
    std::string provider;
    std::string capability;

    return unsignedField(reply.payload, "schemaVersion", schemaVersion) &&
        schemaVersion == 1 &&
        stringField(reply.payload, "provider", provider) &&
        provider == "vdr-plugin-web" &&
        unsignedField(
            reply.payload,
            "providerSchemaVersion",
            providerSchemaVersion) &&
        providerSchemaVersion == 1 &&
        stringField(reply.payload, "capability", capability) &&
        capability == "broadcast.hbbtv.presentation" &&
        stringField(reply.payload, "operation", wire.operation) &&
        wire.operation == expectedOperation &&
        stringField(reply.payload, "result", wire.result) &&
        unsignedField(reply.payload, "resultCode", wire.resultCode) &&
        stringField(reply.payload, "sessionId", wire.sessionId) &&
        wire.sessionId == expectedSession &&
        unsignedField(
            reply.payload,
            "frameRevision",
            wire.frameRevision) &&
        unsignedField(reply.payload, "observedAt", wire.observedAt) &&
        unsignedField(reply.payload, "renderWidth", wire.renderWidth) &&
        unsignedField(reply.payload, "renderHeight", wire.renderHeight) &&
        unsignedField(reply.payload, "encodedBytes", wire.encodedBytes) &&
        unsignedField(reply.payload, "returnedBytes", wire.returnedBytes) &&
        wire.resultCode <= 7U &&
        wire.renderWidth <= MaximumRenderWidth &&
        wire.renderHeight <= MaximumRenderHeight &&
        wire.encodedBytes <= MaximumEncodedBytes &&
        wire.returnedBytes <= PresentationChunkBytes;
}

const char* resultName(std::uint64_t code)
{
    switch (code)
    {
        case 0: return "ok";
        case 1: return "invalid_request";
        case 2: return "no_session";
        case 3: return "session_mismatch";
        case 4: return "no_frame";
        case 5: return "revision_mismatch";
        case 6: return "range_invalid";
        case 7: return "frame_too_large";
    }
    return nullptr;
}

bool resultConsistent(const Wire& wire)
{
    const char* expected = resultName(wire.resultCode);
    return expected != nullptr && wire.result == expected;
}

std::uint32_t bigEndian32(const std::string& bytes, std::size_t offset)
{
    return
        (static_cast<std::uint32_t>(
            static_cast<unsigned char>(bytes[offset])) << 24U) |
        (static_cast<std::uint32_t>(
            static_cast<unsigned char>(bytes[offset + 1U])) << 16U) |
        (static_cast<std::uint32_t>(
            static_cast<unsigned char>(bytes[offset + 2U])) << 8U) |
        static_cast<std::uint32_t>(
            static_cast<unsigned char>(bytes[offset + 3U]));
}

bool validQoi(
    const std::string& bytes,
    std::uint32_t width,
    std::uint32_t height)
{
    return bytes.size() >= 22U &&
        bytes.compare(0, 4, "qoif") == 0 &&
        bigEndian32(bytes, 4) == width &&
        bigEndian32(bytes, 8) == height &&
        static_cast<unsigned char>(bytes[12]) == 4U;
}

} // namespace

SuiteBridgeHbbtvPresentationResolver::SuiteBridgeHbbtvPresentationResolver(
    ISuiteBridgeHbbtvTransport& transport)
    : transport_(transport)
{
}

HbbtvPresentationFrame
SuiteBridgeHbbtvPresentationResolver::readPresentation(
    const std::string& sessionId,
    std::uint64_t knownRevision)
{
    HbbtvPresentationFrame result;
    if (sessionId.empty() || sessionId.size() >= 128U)
    {
        result.error = "hbbtv_presentation_request_invalid";
        return result;
    }

    SuiteBridgeHbbtvPresentationRequest metaRequest;
    metaRequest.operation = SuiteBridgeHbbtvPresentationOperation::Meta;
    metaRequest.sessionId = sessionId;

    Wire meta;
    const SuiteBridgeHbbtvCommandReply metaReply =
        transport_.readHbbtvPresentation(metaRequest);
    if (!parseWire(metaReply, "meta", sessionId, meta) ||
        !resultConsistent(meta))
    {
        result.error = "hbbtv_presentation_payload_invalid";
        return result;
    }

    if (meta.resultCode == 2U || meta.resultCode == 4U)
    {
        return result;
    }
    if (meta.resultCode != 0U)
    {
        result.error = "hbbtv_presentation_" + meta.result;
        return result;
    }

    if (meta.frameRevision == 0 ||
        meta.renderWidth == 0 ||
        meta.renderHeight == 0 ||
        meta.encodedBytes == 0)
    {
        result.error = "hbbtv_presentation_payload_invalid";
        return result;
    }

    result.frameRevision = meta.frameRevision;
    result.observedAt = meta.observedAt;
    result.renderWidth = static_cast<std::uint32_t>(meta.renderWidth);
    result.renderHeight = static_cast<std::uint32_t>(meta.renderHeight);

    if (knownRevision == meta.frameRevision)
    {
        result.available = true;
        result.unchanged = true;
        return result;
    }

    std::string encoded;
    encoded.reserve(static_cast<std::size_t>(meta.encodedBytes));

    std::uint32_t offset = 0;
    while (encoded.size() < meta.encodedBytes)
    {
        SuiteBridgeHbbtvPresentationRequest chunkRequest;
        chunkRequest.operation =
            SuiteBridgeHbbtvPresentationOperation::Chunk;
        chunkRequest.sessionId = sessionId;
        chunkRequest.frameRevision = meta.frameRevision;
        chunkRequest.offset = offset;

        Wire chunk;
        const SuiteBridgeHbbtvCommandReply chunkReply =
            transport_.readHbbtvPresentation(chunkRequest);
        if (!parseWire(chunkReply, "chunk", sessionId, chunk) ||
            !resultConsistent(chunk))
        {
            result.error = "hbbtv_presentation_payload_invalid";
            return result;
        }

        if (chunk.resultCode == 5U)
        {
            result.error = "hbbtv_presentation_changed";
            return result;
        }
        if (chunk.resultCode != 0U ||
            chunk.frameRevision != meta.frameRevision ||
            chunk.renderWidth != meta.renderWidth ||
            chunk.renderHeight != meta.renderHeight ||
            chunk.encodedBytes != meta.encodedBytes ||
            chunk.returnedBytes == 0)
        {
            result.error = "hbbtv_presentation_payload_invalid";
            return result;
        }

        if (!stringField(
                chunkReply.payload,
                "dataBase64",
                chunk.dataBase64) ||
            chunk.dataBase64.empty())
        {
            result.error = "hbbtv_presentation_payload_invalid";
            return result;
        }

        std::string decoded;
        if (!decodeBase64(chunk.dataBase64, decoded) ||
            decoded.size() != chunk.returnedBytes ||
            encoded.size() + decoded.size() > meta.encodedBytes)
        {
            result.error = "hbbtv_presentation_payload_invalid";
            return result;
        }

        encoded.append(decoded);
        if (encoded.size() >
            std::numeric_limits<std::uint32_t>::max())
        {
            result.error = "hbbtv_presentation_payload_invalid";
            return result;
        }
        offset = static_cast<std::uint32_t>(encoded.size());
    }

    if (encoded.size() != meta.encodedBytes ||
        !validQoi(
            encoded,
            result.renderWidth,
            result.renderHeight))
    {
        result.error = "hbbtv_presentation_qoi_invalid";
        return result;
    }

    result.available = true;
    result.qoi = std::move(encoded);
    return result;
}
