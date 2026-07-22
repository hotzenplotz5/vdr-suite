#pragma once

#include "VdrRecordingNativeMetadata.h"

#include <cstdint>
#include <string>
#include <vector>

class Database;
struct sqlite3;
struct sqlite3_stmt;

namespace vdr_recording_native_repository_detail
{

class Statement final
{
public:
    Statement(sqlite3* database, const char* sql);
    ~Statement();
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    sqlite3_stmt* get() const noexcept;
    bool valid() const noexcept;
private:
    sqlite3_stmt* statement_ = nullptr;
};

class Transaction final
{
public:
    explicit Transaction(Database& database);
    ~Transaction();
    bool active() const noexcept;
    bool commit();
private:
    Database& database_;
    bool active_ = false;
};

bool bindText(sqlite3_stmt* statement, int index, const std::string& value);
std::string columnText(sqlite3_stmt* statement, int column);
std::string availabilityText(VdrRecordingNativeMetadataAvailability availability);
VdrRecordingNativeMetadataAvailability availabilityFromState(
    const std::string& contentState,
    const std::string& lastAttemptState);
std::string folded(const std::string& value);
std::string normalizedPersonName(const std::string& value);
bool validRecordingKey(const std::string& key);
bool executeDelete(
    sqlite3* database,
    const char* sql,
    const std::string& backendId,
    const std::string& recordingKey);
bool deleteChildren(
    sqlite3* database,
    const std::string& backendId,
    const std::string& recordingKey);
bool insertTextList(
    sqlite3* database,
    const std::string& backendId,
    const std::string& recordingKey,
    const std::string& kind,
    const std::vector<std::string>& values);
bool insertPeople(
    sqlite3* database,
    const std::string& backendId,
    const std::string& recordingKey,
    const std::vector<VdrRecordingNativePerson>& people);
bool insertImages(
    sqlite3* database,
    const std::string& backendId,
    const std::string& recordingKey,
    const std::vector<VdrRecordingNativeArtwork>& images);
std::vector<std::string> loadTextList(
    sqlite3* database,
    const std::string& backendId,
    const std::string& recordingKey,
    const std::string& kind);
std::vector<VdrRecordingNativePerson> loadPeople(
    sqlite3* database,
    const std::string& backendId,
    const std::string& recordingKey);
std::vector<VdrRecordingNativeArtwork> loadImages(
    sqlite3* database,
    const std::string& backendId,
    const std::string& recordingKey);

}
