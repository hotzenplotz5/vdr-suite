#include "MetadataIdentity.h"

#include <array>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <utility>

namespace
{

constexpr const char* metadataEntityPrefix = "mdent_";
constexpr const char* metadataAssignmentPrefix = "mdasg_";
constexpr const char* metadataTargetPrefix = "mdtgt_";
constexpr std::size_t opaqueIdHexLength = 32;

bool isLowerHex(const char character)
{
    return (character >= '0' && character <= '9') ||
           (character >= 'a' && character <= 'f');
}

bool isValidOpaqueId(
    const std::string& value,
    const std::string& prefix)
{
    if (value.size() != prefix.size() + opaqueIdHexLength ||
        value.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    for (std::size_t index = prefix.size();
         index < value.size();
         ++index)
    {
        if (!isLowerHex(value[index]))
        {
            return false;
        }
    }

    return true;
}

std::string generateOpaqueId(const std::string& prefix)
{
    std::random_device randomDevice;
    std::array<unsigned char, 16> bytes{};

    for (unsigned char& byte : bytes)
    {
        byte = static_cast<unsigned char>(randomDevice() & 0xffu);
    }

    std::ostringstream value;
    value << prefix << std::hex << std::setfill('0');

    for (const unsigned char byte : bytes)
    {
        value << std::setw(2) << static_cast<unsigned int>(byte);
    }

    return value.str();
}

bool isProviderCharacter(const char character)
{
    return (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') ||
           character == '-' ||
           character == '_' ||
           character == '.';
}

bool isProviderBoundaryCharacter(const char character)
{
    return (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9');
}

}

const char* metadataMediaTypeName(const MetadataMediaType type)
{
    switch (type)
    {
    case MetadataMediaType::Movie:
        return "movie";
    case MetadataMediaType::Series:
        return "series";
    case MetadataMediaType::Season:
        return "season";
    case MetadataMediaType::Episode:
        return "episode";
    case MetadataMediaType::Programme:
        return "programme";
    case MetadataMediaType::Person:
        return "person";
    case MetadataMediaType::Unknown:
    default:
        return "unknown";
    }
}

const char* metadataTargetTypeName(const MetadataTargetType type)
{
    switch (type)
    {
    case MetadataTargetType::Recording:
        return "recording";
    case MetadataTargetType::ProgramEvent:
        return "program-event";
    case MetadataTargetType::TimerIntent:
        return "timer-intent";
    case MetadataTargetType::Unknown:
    default:
        return "unknown";
    }
}

MetadataEntityId::MetadataEntityId(std::string value)
    : value_(std::move(value))
{
}

MetadataEntityId MetadataEntityId::generate()
{
    return MetadataEntityId(generateOpaqueId(metadataEntityPrefix));
}

bool MetadataEntityId::isValidValue(const std::string& value)
{
    return isValidOpaqueId(value, metadataEntityPrefix);
}

bool MetadataEntityId::isValid() const
{
    return isValidValue(value_);
}

bool MetadataEntityId::empty() const
{
    return value_.empty();
}

const std::string& MetadataEntityId::value() const
{
    return value_;
}

bool MetadataEntityId::operator==(const MetadataEntityId& other) const
{
    return value_ == other.value_;
}

bool MetadataEntityId::operator!=(const MetadataEntityId& other) const
{
    return !(*this == other);
}

bool MetadataEntityId::operator<(const MetadataEntityId& other) const
{
    return value_ < other.value_;
}

MetadataAssignmentId::MetadataAssignmentId(std::string value)
    : value_(std::move(value))
{
}

MetadataAssignmentId MetadataAssignmentId::generate()
{
    return MetadataAssignmentId(
        generateOpaqueId(metadataAssignmentPrefix));
}

bool MetadataAssignmentId::isValidValue(const std::string& value)
{
    return isValidOpaqueId(value, metadataAssignmentPrefix);
}

bool MetadataAssignmentId::isValid() const
{
    return isValidValue(value_);
}

bool MetadataAssignmentId::empty() const
{
    return value_.empty();
}

const std::string& MetadataAssignmentId::value() const
{
    return value_;
}

bool MetadataAssignmentId::operator==(
    const MetadataAssignmentId& other) const
{
    return value_ == other.value_;
}

bool MetadataAssignmentId::operator!=(
    const MetadataAssignmentId& other) const
{
    return !(*this == other);
}

bool MetadataAssignmentId::operator<(
    const MetadataAssignmentId& other) const
{
    return value_ < other.value_;
}

MetadataTargetId::MetadataTargetId(std::string value)
    : value_(std::move(value))
{
}

MetadataTargetId MetadataTargetId::generate()
{
    return MetadataTargetId(generateOpaqueId(metadataTargetPrefix));
}

bool MetadataTargetId::isValidValue(const std::string& value)
{
    return isValidOpaqueId(value, metadataTargetPrefix);
}

bool MetadataTargetId::isValid() const
{
    return isValidValue(value_);
}

bool MetadataTargetId::empty() const
{
    return value_.empty();
}

const std::string& MetadataTargetId::value() const
{
    return value_;
}

bool MetadataTargetId::operator==(const MetadataTargetId& other) const
{
    return value_ == other.value_;
}

bool MetadataTargetId::operator!=(const MetadataTargetId& other) const
{
    return !(*this == other);
}

bool MetadataTargetId::operator<(const MetadataTargetId& other) const
{
    return value_ < other.value_;
}

MetadataProviderId::MetadataProviderId(std::string value)
    : value_(std::move(value))
{
}

bool MetadataProviderId::isValidValue(const std::string& value)
{
    if (value.empty() || value.size() > 64 ||
        !isProviderBoundaryCharacter(value.front()) ||
        !isProviderBoundaryCharacter(value.back()))
    {
        return false;
    }

    for (const char character : value)
    {
        if (!isProviderCharacter(character))
        {
            return false;
        }
    }

    return true;
}

bool MetadataProviderId::isValid() const
{
    return isValidValue(value_);
}

bool MetadataProviderId::empty() const
{
    return value_.empty();
}

const std::string& MetadataProviderId::value() const
{
    return value_;
}

bool MetadataProviderId::operator==(const MetadataProviderId& other) const
{
    return value_ == other.value_;
}

bool MetadataProviderId::operator!=(const MetadataProviderId& other) const
{
    return !(*this == other);
}

bool MetadataProviderId::operator<(const MetadataProviderId& other) const
{
    return value_ < other.value_;
}

bool MetadataTargetRef::isValid() const
{
    return type != MetadataTargetType::Unknown && targetId.isValid();
}

std::string MetadataTargetRef::canonicalKey() const
{
    if (!isValid())
    {
        return {};
    }

    return std::string(metadataTargetTypeName(type)) + ":" +
           targetId.value();
}

bool MetadataTargetRef::operator==(const MetadataTargetRef& other) const
{
    return type == other.type && targetId == other.targetId;
}

bool MetadataTargetRef::operator!=(const MetadataTargetRef& other) const
{
    return !(*this == other);
}

bool MetadataTargetRef::operator<(const MetadataTargetRef& other) const
{
    if (type != other.type)
    {
        return static_cast<int>(type) < static_cast<int>(other.type);
    }

    return targetId < other.targetId;
}
