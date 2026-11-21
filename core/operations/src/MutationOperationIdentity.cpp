#include "MutationOperationIdentity.h"

#include <array>
#include <cstddef>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace vdrsuite::operations
{
namespace
{
constexpr const char* kPrefix = "op_";
constexpr std::size_t kRandomBytes = 16;
constexpr std::size_t kHexLength = kRandomBytes * 2;

bool lowerHex(char character)
{
    return (character >= '0' && character <= '9')
        || (character >= 'a' && character <= 'f');
}

std::string generate()
{
    try
    {
        std::random_device randomDevice;
        std::array<unsigned char, kRandomBytes> bytes{};
        for (unsigned char& byte : bytes)
            byte = static_cast<unsigned char>(randomDevice() & 0xffu);

        std::ostringstream output;
        output << kPrefix << std::hex << std::setfill('0');
        for (unsigned char byte : bytes)
            output << std::setw(2) << static_cast<unsigned int>(byte);
        return output.str();
    }
    catch (...)
    {
        return {};
    }
}
}

std::string generateMutationOperationId()
{
    return generate();
}

bool mutationOperationIdCanonical(const std::string& value)
{
    const std::string prefix = kPrefix;
    if (value.size() != prefix.size() + kHexLength
        || value.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    for (std::size_t index = prefix.size(); index < value.size(); ++index)
        if (!lowerHex(value[index])) return false;

    return true;
}

} // namespace vdrsuite::operations
