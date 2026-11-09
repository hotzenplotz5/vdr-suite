#pragma once

#include <string>

namespace vdrsuite::agent
{
// Encode HTTP Basic credentials for the existing Agent Authorization header.
// The result contains credentials: never log it or expose it in an error.
// This does not change the server's Basic authentication contract.
inline std::string backendAgentBasicAuthorizationValue(
    const std::string& login, const std::string& secret)
{
    static constexpr char Alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const std::string value = login + ":" + secret;
    std::string encoded;
    encoded.reserve(((value.size() + 2U) / 3U) * 4U + 6U);
    encoded = "Basic ";
    for (std::size_t offset = 0; offset < value.size(); offset += 3U)
    {
        const std::size_t remaining = value.size() - offset;
        const unsigned int first =
            static_cast<unsigned char>(value[offset]);
        const unsigned int second = remaining > 1U
            ? static_cast<unsigned char>(value[offset + 1U]) : 0U;
        const unsigned int third = remaining > 2U
            ? static_cast<unsigned char>(value[offset + 2U]) : 0U;
        const unsigned int packed =
            (first << 16U) | (second << 8U) | third;
        encoded.push_back(Alphabet[(packed >> 18U) & 0x3fU]);
        encoded.push_back(Alphabet[(packed >> 12U) & 0x3fU]);
        encoded.push_back(remaining > 1U
            ? Alphabet[(packed >> 6U) & 0x3fU] : '=');
        encoded.push_back(remaining > 2U
            ? Alphabet[packed & 0x3fU] : '=');
    }
    return encoded;
}
}
