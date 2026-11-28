#pragma once

#include <openssl/sha.h>

#include <array>
#include <string>

// One presentation key for cached Series members and persisted Web cover
// settings. The legacy key remains the settings owner's storage identity.
namespace PublicSeriesGroupKey
{
inline std::string fromLegacy(
    const std::string& backendId, const std::string& legacyKey)
{
    if (backendId.empty() || legacyKey.empty()) return {};
    const std::string input = "recording-series-group/1|" + backendId +
        std::string(1, '\0') + legacyKey;
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    if (!SHA256(reinterpret_cast<const unsigned char*>(input.data()),
                input.size(), digest.data())) return {};
    constexpr char hex[] = "0123456789abcdef";
    std::string out = "sg1_";
    for (std::size_t i = 0; i < 16U; ++i)
    {
        const auto byte = digest[i];
        out += hex[byte >> 4U];
        out += hex[byte & 15U];
    }
    return out;
}
}
