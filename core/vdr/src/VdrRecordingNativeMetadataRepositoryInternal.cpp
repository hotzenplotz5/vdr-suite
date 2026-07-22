#include "VdrRecordingNativeMetadataRepositoryInternal.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <utility>

namespace vdr_recording_native_repository_detail
{

Statement::Statement(sqlite3* database, const char* sql)
{
    if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK)
    {
        statement_ = nullptr;
    }
}

Statement::~Statement()
{
    sqlite3_finalize(statement_);
}

sqlite3_stmt* Statement::get() const noexcept
{
    return statement_;
}

bool Statement::valid() const noexcept
{
    return statement_ != nullptr;
}

Transaction::Transaction(Database& database)
    : database_(database),
      active_(database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
{
}

Transaction::~Transaction()
{
    if (active_)
    {
        database_.execute("ROLLBACK;");
    }
}

bool Transaction::active() const noexcept
{
    return active_;
}

bool Transaction::commit()
{
    if (!active_ || !database_.execute("COMMIT;"))
    {
        return false;
    }
    active_ = false;
    return true;
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : std::string{};
}

std::string availabilityText(VdrRecordingNativeMetadataAvailability availability)
{
    switch (availability)
    {
        case VdrRecordingNativeMetadataAvailability::Found: return "found";
        case VdrRecordingNativeMetadataAvailability::NotFound: return "not_found";
        case VdrRecordingNativeMetadataAvailability::ProviderUnavailable: return "provider_unavailable";
        case VdrRecordingNativeMetadataAvailability::TransportError: return "transport_error";
        case VdrRecordingNativeMetadataAvailability::InvalidPayload: return "invalid_payload";
    }
    return "invalid_payload";
}

VdrRecordingNativeMetadataAvailability availabilityFromState(
    const std::string& contentState,
    const std::string& lastAttemptState)
{
    if (contentState == "found") return VdrRecordingNativeMetadataAvailability::Found;
    if (contentState == "not_found") return VdrRecordingNativeMetadataAvailability::NotFound;
    if (lastAttemptState == "provider_unavailable") return VdrRecordingNativeMetadataAvailability::ProviderUnavailable;
    if (lastAttemptState == "transport_error") return VdrRecordingNativeMetadataAvailability::TransportError;
    return VdrRecordingNativeMetadataAvailability::InvalidPayload;
}

std::string folded(const std::string& value)
{
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

std::string normalizedPersonName(const std::string& value)
{
    std::string result;
    bool separatorPending = false;
    for (const unsigned char character : value)
    {
        if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9'))
        {
            if (separatorPending && !result.empty()) result.push_back('-');
            separatorPending = false;
            result.push_back(static_cast<char>(character));
        }
        else if (character >= 'A' && character <= 'Z')
        {
            if (separatorPending && !result.empty()) result.push_back('-');
            separatorPending = false;
            result.push_back(static_cast<char>(character - 'A' + 'a'));
        }
        else if (character >= 0x80)
        {
            if (separatorPending && !result.empty()) result.push_back('-');
            separatorPending = false;
            result.push_back(static_cast<char>(character));
        }
        else
        {
            separatorPending = !result.empty();
        }
    }
    return result;
}

bool validRecordingKey(const std::string& key)
{
    return key.size() == 32 && std::all_of(key.begin(), key.end(), [](unsigned char character) {
        return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
    });
}

bool executeDelete(sqlite3* database, const char* sql, const std::string& backendId, const std::string& recordingKey)
{
    Statement statement(database, sql);
    return statement.valid() && bindText(statement.get(), 1, backendId) && bindText(statement.get(), 2, recordingKey) && sqlite3_step(statement.get()) == SQLITE_DONE;
}

bool deleteChildren(sqlite3* database, const std::string& backendId, const std::string& recordingKey)
{
    return executeDelete(database, "DELETE FROM vdr_recording_native_text_list WHERE backend_id = ? AND recording_key = ?;", backendId, recordingKey) &&
        executeDelete(database, "DELETE FROM vdr_recording_native_person WHERE backend_id = ? AND recording_key = ?;", backendId, recordingKey) &&
        executeDelete(database, "DELETE FROM vdr_recording_native_artwork WHERE backend_id = ? AND recording_key = ?;", backendId, recordingKey);
}

bool insertTextList(sqlite3* database, const std::string& backendId, const std::string& recordingKey, const std::string& kind, const std::vector<std::string>& values)
{
    Statement statement(database, "INSERT INTO vdr_recording_native_text_list (backend_id, recording_key, kind, ordinal, value) VALUES (?, ?, ?, ?, ?);");
    if (!statement.valid()) return false;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        sqlite3_reset(statement.get()); sqlite3_clear_bindings(statement.get());
        if (!bindText(statement.get(), 1, backendId) || !bindText(statement.get(), 2, recordingKey) || !bindText(statement.get(), 3, kind) ||
            sqlite3_bind_int(statement.get(), 4, static_cast<int>(index)) != SQLITE_OK || !bindText(statement.get(), 5, values[index]) || sqlite3_step(statement.get()) != SQLITE_DONE) return false;
    }
    return true;
}

bool insertPeople(sqlite3* database, const std::string& backendId, const std::string& recordingKey, const std::vector<VdrRecordingNativePerson>& people)
{
    Statement statement(database, "INSERT INTO vdr_recording_native_person (backend_id, recording_key, ordinal, role, name, name_folded, normalized_name, character_name, character_name_folded, image_provider, image_path, image_width, image_height) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    if (!statement.valid()) return false;
    for (std::size_t index = 0; index < people.size(); ++index)
    {
        const auto& person = people[index]; sqlite3_reset(statement.get()); sqlite3_clear_bindings(statement.get());
        const bool bound = bindText(statement.get(), 1, backendId) && bindText(statement.get(), 2, recordingKey) &&
            sqlite3_bind_int(statement.get(), 3, static_cast<int>(index)) == SQLITE_OK && bindText(statement.get(), 4, person.role) &&
            bindText(statement.get(), 5, person.name) && bindText(statement.get(), 6, folded(person.name)) &&
            bindText(statement.get(), 7, normalizedPersonName(person.name)) && bindText(statement.get(), 8, person.characterName) &&
            bindText(statement.get(), 9, folded(person.characterName)) && bindText(statement.get(), 10, person.image.provider) &&
            bindText(statement.get(), 11, person.image.path) && sqlite3_bind_int(statement.get(), 12, person.image.width) == SQLITE_OK &&
            sqlite3_bind_int(statement.get(), 13, person.image.height) == SQLITE_OK;
        if (!bound || sqlite3_step(statement.get()) != SQLITE_DONE) return false;
    }
    return true;
}

bool insertImages(sqlite3* database, const std::string& backendId, const std::string& recordingKey, const std::vector<VdrRecordingNativeArtwork>& images)
{
    Statement statement(database, "INSERT INTO vdr_recording_native_artwork (backend_id, recording_key, ordinal, orientation, provider, path, width, height) VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    if (!statement.valid()) return false;
    for (std::size_t index = 0; index < images.size(); ++index)
    {
        const auto& image = images[index]; sqlite3_reset(statement.get()); sqlite3_clear_bindings(statement.get());
        const bool bound = bindText(statement.get(), 1, backendId) && bindText(statement.get(), 2, recordingKey) &&
            sqlite3_bind_int(statement.get(), 3, static_cast<int>(index)) == SQLITE_OK && bindText(statement.get(), 4, image.orientation) &&
            bindText(statement.get(), 5, image.provider) && bindText(statement.get(), 6, image.path) &&
            sqlite3_bind_int(statement.get(), 7, image.width) == SQLITE_OK && sqlite3_bind_int(statement.get(), 8, image.height) == SQLITE_OK;
        if (!bound || sqlite3_step(statement.get()) != SQLITE_DONE) return false;
    }
    return true;
}

std::vector<std::string> loadTextList(sqlite3* database, const std::string& backendId, const std::string& recordingKey, const std::string& kind)
{
    std::vector<std::string> values;
    Statement statement(database, "SELECT value FROM vdr_recording_native_text_list WHERE backend_id = ? AND recording_key = ? AND kind = ? ORDER BY ordinal ASC;");
    if (!statement.valid() || !bindText(statement.get(), 1, backendId) || !bindText(statement.get(), 2, recordingKey) || !bindText(statement.get(), 3, kind)) return values;
    while (sqlite3_step(statement.get()) == SQLITE_ROW) values.push_back(columnText(statement.get(), 0));
    return values;
}

std::vector<VdrRecordingNativePerson> loadPeople(sqlite3* database, const std::string& backendId, const std::string& recordingKey)
{
    std::vector<VdrRecordingNativePerson> people;
    Statement statement(database, "SELECT role, name, character_name, image_provider, image_path, image_width, image_height FROM vdr_recording_native_person WHERE backend_id = ? AND recording_key = ? ORDER BY ordinal ASC;");
    if (!statement.valid() || !bindText(statement.get(), 1, backendId) || !bindText(statement.get(), 2, recordingKey)) return people;
    while (sqlite3_step(statement.get()) == SQLITE_ROW)
    {
        VdrRecordingNativePerson person; person.role = columnText(statement.get(), 0); person.name = columnText(statement.get(), 1); person.characterName = columnText(statement.get(), 2);
        person.image.provider = columnText(statement.get(), 3); person.image.path = columnText(statement.get(), 4); person.image.width = sqlite3_column_int(statement.get(), 5); person.image.height = sqlite3_column_int(statement.get(), 6); person.image.available = !person.image.path.empty();
        people.push_back(std::move(person));
    }
    return people;
}

std::vector<VdrRecordingNativeArtwork> loadImages(sqlite3* database, const std::string& backendId, const std::string& recordingKey)
{
    std::vector<VdrRecordingNativeArtwork> images;
    Statement statement(database, "SELECT orientation, provider, path, width, height FROM vdr_recording_native_artwork WHERE backend_id = ? AND recording_key = ? ORDER BY ordinal ASC;");
    if (!statement.valid() || !bindText(statement.get(), 1, backendId) || !bindText(statement.get(), 2, recordingKey)) return images;
    while (sqlite3_step(statement.get()) == SQLITE_ROW)
    {
        VdrRecordingNativeArtwork image; image.orientation = columnText(statement.get(), 0); image.provider = columnText(statement.get(), 1); image.path = columnText(statement.get(), 2); image.width = sqlite3_column_int(statement.get(), 3); image.height = sqlite3_column_int(statement.get(), 4); image.available = !image.path.empty();
        images.push_back(std::move(image));
    }
    return images;
}

}
