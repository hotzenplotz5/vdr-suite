#pragma once

#include <cstddef>
#include <string>

class VdrRecordingNativeIdentity final
{
public:
    static constexpr std::size_t MaximumNativeIdBytes = 4096;
    static constexpr std::size_t KeyBytes = 32;

    static std::string keyForNativeId(const std::string& nativeId);
    static bool isValidKey(const std::string& key) noexcept;
};
