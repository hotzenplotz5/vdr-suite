#pragma once

#include <string>

enum class MetadataMediaType
{
    Unknown,
    Movie,
    Series,
    Season,
    Episode,
    Programme,
    Person
};

enum class MetadataTargetType
{
    Unknown,
    Recording,
    ProgramEvent,
    TimerIntent
};

const char* metadataMediaTypeName(MetadataMediaType type);
const char* metadataTargetTypeName(MetadataTargetType type);

class MetadataEntityId
{
public:
    MetadataEntityId() = default;
    explicit MetadataEntityId(std::string value);

    static MetadataEntityId generate();
    static bool isValidValue(const std::string& value);

    bool isValid() const;
    bool empty() const;
    const std::string& value() const;

    bool operator==(const MetadataEntityId& other) const;
    bool operator!=(const MetadataEntityId& other) const;
    bool operator<(const MetadataEntityId& other) const;

private:
    std::string value_;
};

class MetadataAssignmentId
{
public:
    MetadataAssignmentId() = default;
    explicit MetadataAssignmentId(std::string value);

    static MetadataAssignmentId generate();
    static bool isValidValue(const std::string& value);

    bool isValid() const;
    bool empty() const;
    const std::string& value() const;

    bool operator==(const MetadataAssignmentId& other) const;
    bool operator!=(const MetadataAssignmentId& other) const;
    bool operator<(const MetadataAssignmentId& other) const;

private:
    std::string value_;
};

class MetadataTargetId
{
public:
    MetadataTargetId() = default;
    explicit MetadataTargetId(std::string value);

    static MetadataTargetId generate();
    static bool isValidValue(const std::string& value);

    bool isValid() const;
    bool empty() const;
    const std::string& value() const;

    bool operator==(const MetadataTargetId& other) const;
    bool operator!=(const MetadataTargetId& other) const;
    bool operator<(const MetadataTargetId& other) const;

private:
    std::string value_;
};

class MetadataProviderId
{
public:
    MetadataProviderId() = default;
    explicit MetadataProviderId(std::string value);

    static bool isValidValue(const std::string& value);

    bool isValid() const;
    bool empty() const;
    const std::string& value() const;

    bool operator==(const MetadataProviderId& other) const;
    bool operator!=(const MetadataProviderId& other) const;
    bool operator<(const MetadataProviderId& other) const;

private:
    std::string value_;
};

struct MetadataTargetRef
{
    MetadataTargetType type = MetadataTargetType::Unknown;
    MetadataTargetId targetId;

    bool isValid() const;
    std::string canonicalKey() const;

    bool operator==(const MetadataTargetRef& other) const;
    bool operator!=(const MetadataTargetRef& other) const;
    bool operator<(const MetadataTargetRef& other) const;
};
