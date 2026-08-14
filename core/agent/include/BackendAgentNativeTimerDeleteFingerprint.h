#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>

namespace vdrsuite::agent
{

constexpr std::size_t kBackendAgentNativeTimerDeleteCanonicalFingerprintMaximum = 4096;
constexpr std::size_t kBackendAgentNativeTimerDeleteFingerprintTokenMaximum =
    kBackendAgentNativeTimerDeleteCanonicalFingerprintMaximum * 2;

inline bool backendAgentNativeTimerDeleteCanonicalFingerprintValid(
    const std::string& value)
{
    return !value.empty() &&
        value.size() <= kBackendAgentNativeTimerDeleteCanonicalFingerprintMaximum &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= 0x20U && character != 0x7fU;
        });
}

inline std::string backendAgentNativeTimerDeleteFingerprintToken(
    const std::string& canonicalFingerprint)
{
    if (!backendAgentNativeTimerDeleteCanonicalFingerprintValid(
            canonicalFingerprint))
        return {};

    static constexpr char Hex[] = "0123456789abcdef";
    std::string token;
    token.resize(canonicalFingerprint.size() * 2);
    for (std::size_t index = 0; index < canonicalFingerprint.size(); ++index)
    {
        const unsigned char value =
            static_cast<unsigned char>(canonicalFingerprint[index]);
        token[index * 2] = Hex[value >> 4U];
        token[index * 2 + 1] = Hex[value & 0x0fU];
    }
    return token;
}

inline bool backendAgentNativeTimerDeleteFingerprintTokenValid(
    const std::string& value)
{
    return !value.empty() &&
        value.size() <= kBackendAgentNativeTimerDeleteFingerprintTokenMaximum &&
        value.size() % 2 == 0 &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return (character >= '0' && character <= '9') ||
                (character >= 'a' && character <= 'f');
        });
}

} // namespace vdrsuite::agent
