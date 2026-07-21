#pragma once

#include <algorithm>
#include <cctype>
#include <string>

class PersonNameNormalizer
{
public:
    static std::string normalize(const std::string& name)
    {
        std::string normalized;
        bool previousWasSeparator = false;

        for (const char character : name)
        {
            const unsigned char value =
                static_cast<unsigned char>(character);

            if (std::isalnum(value))
            {
                normalized.push_back(
                    static_cast<char>(std::tolower(value)));
                previousWasSeparator = false;
            }
            else if (!previousWasSeparator && !normalized.empty())
            {
                normalized.push_back('-');
                previousWasSeparator = true;
            }
        }

        while (!normalized.empty() && normalized.back() == '-')
        {
            normalized.pop_back();
        }

        return normalized;
    }
};
