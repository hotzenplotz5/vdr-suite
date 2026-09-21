#pragma once

#include <crypt.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>

namespace BrowserSessionCsrfToken
{
inline void wipeCryptData(crypt_data& data) noexcept
{
    volatile unsigned char* bytes =
        reinterpret_cast<volatile unsigned char*>(&data);
    for (std::size_t index = 0; index < sizeof(data); ++index)
    {
        bytes[index] = 0;
    }
}

inline std::string derive(const std::string& csrfSecretHash)
{
    if (csrfSecretHash.empty())
    {
        return {};
    }

    crypt_data data{};
    char* encoded = crypt_r(
        csrfSecretHash.c_str(),
        "$6$rounds=10000$vdrsuitecsrf$",
        &data);

    std::string derived;
    if (encoded != nullptr && encoded[0] != '*')
    {
        const std::string value(encoded);
        const std::size_t digestSeparator = value.rfind('$');
        if (digestSeparator != std::string::npos &&
            digestSeparator + 1 < value.size())
        {
            derived = value.substr(digestSeparator + 1);
        }
    }
    wipeCryptData(data);

    for (char& character : derived)
    {
        if (character == '.')
        {
            character = '-';
        }
        else if (character == '/')
        {
            character = '_';
        }
    }

    const bool safe =
        derived.size() >= 32 &&
        derived.size() <= 256 &&
        std::all_of(
            derived.begin(),
            derived.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '-' ||
                    character == '_';
            });

    if (!safe)
    {
        std::fill(derived.begin(), derived.end(), '\0');
        derived.clear();
    }
    return derived;
}
}
