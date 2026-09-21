#pragma once

#include <cctype>
#include <string>
#include <vector>

struct RestfulApiRecordingTrashPreviewResponse
{
    bool parsed = false;
    bool executable = false;
    std::string recordingFile;
    long long recordingsState = 0;
    long long timersState = 0;
    std::vector<std::string> blockers;
    std::vector<std::string> warnings;
};

struct RestfulApiRecordingTrashValidateResponse
{
    bool parsed = false;
    std::string status;
    std::vector<std::string> blockers;
    std::vector<std::string> warnings;
};

struct RestfulApiRecordingTrashExecuteResponse
{
    bool parsed = false;
    std::string status;
    std::string recordingFile;
    std::string deletedRecordingFile;
    std::string message;
};

class RestfulApiRecordingTrashResponseParser
{
public:
    static RestfulApiRecordingTrashPreviewResponse parsePreview(
        const std::string& body)
    {
        RestfulApiRecordingTrashPreviewResponse result;
        const bool executableParsed =
            parseBooleanField(body, "executable", result.executable);
        const bool recordingsStateParsed =
            parseIntegerField(
                body,
                "revision_recordings_state",
                result.recordingsState);
        const bool timersStateParsed =
            parseIntegerField(
                body,
                "revision_timers_state",
                result.timersState);

        parseStringField(body, "recording_file", result.recordingFile);
        parseStringArrayField(body, "blockers", result.blockers);
        parseStringArrayField(body, "warnings", result.warnings);

        result.parsed =
            executableParsed &&
            recordingsStateParsed &&
            timersStateParsed;
        return result;
    }

    static RestfulApiRecordingTrashValidateResponse parseValidate(
        const std::string& body)
    {
        RestfulApiRecordingTrashValidateResponse result;
        result.parsed = parseStringField(body, "status", result.status);
        parseStringArrayField(body, "blockers", result.blockers);
        parseStringArrayField(body, "warnings", result.warnings);
        return result;
    }

    static RestfulApiRecordingTrashExecuteResponse parseExecute(
        const std::string& body)
    {
        RestfulApiRecordingTrashExecuteResponse result;
        result.parsed = parseStringField(body, "status", result.status);
        parseStringField(body, "recording_file", result.recordingFile);
        parseStringField(
            body,
            "deleted_recording_file",
            result.deletedRecordingFile);
        parseStringField(body, "message", result.message);
        return result;
    }

private:
    static void skipWhitespace(
        const std::string& text,
        std::size_t& position)
    {
        while (position < text.size() &&
               std::isspace(
                   static_cast<unsigned char>(text[position]))) {
            ++position;
        }
    }

    static bool findFieldValue(
        const std::string& body,
        const std::string& fieldName,
        std::size_t& position)
    {
        const std::string marker = "\"" + fieldName + "\"";
        const std::size_t field = body.find(marker);
        if (field == std::string::npos) {
            return false;
        }

        const std::size_t colon =
            body.find(':', field + marker.size());
        if (colon == std::string::npos) {
            return false;
        }

        position = colon + 1;
        skipWhitespace(body, position);
        return position < body.size();
    }

    static bool parseStringValue(
        const std::string& body,
        std::size_t& position,
        std::string& value)
    {
        skipWhitespace(body, position);
        if (position >= body.size() || body[position] != '\"') {
            return false;
        }

        ++position;
        std::string parsed;
        bool escaped = false;

        while (position < body.size()) {
            const char character = body[position++];
            if (escaped) {
                switch (character) {
                    case 'n': parsed.push_back('\n'); break;
                    case 'r': parsed.push_back('\r'); break;
                    case 't': parsed.push_back('\t'); break;
                    default: parsed.push_back(character); break;
                }
                escaped = false;
                continue;
            }

            if (character == '\\') {
                escaped = true;
                continue;
            }

            if (character == '\"') {
                value = parsed;
                return true;
            }

            parsed.push_back(character);
        }

        return false;
    }

    static bool parseStringField(
        const std::string& body,
        const std::string& fieldName,
        std::string& value)
    {
        std::size_t position = 0;
        return findFieldValue(body, fieldName, position) &&
            parseStringValue(body, position, value);
    }

    static bool parseBooleanField(
        const std::string& body,
        const std::string& fieldName,
        bool& value)
    {
        std::size_t position = 0;
        if (!findFieldValue(body, fieldName, position)) {
            return false;
        }

        if (body.compare(position, 4, "true") == 0) {
            value = true;
            return true;
        }

        if (body.compare(position, 5, "false") == 0) {
            value = false;
            return true;
        }

        return false;
    }

    static bool parseIntegerField(
        const std::string& body,
        const std::string& fieldName,
        long long& value)
    {
        std::size_t position = 0;
        if (!findFieldValue(body, fieldName, position)) {
            return false;
        }

        bool quoted = false;
        if (body[position] == '\"') {
            quoted = true;
            ++position;
        }

        const std::size_t start = position;
        if (position < body.size() && body[position] == '-') {
            ++position;
        }

        const std::size_t digitsStart = position;
        while (position < body.size() &&
               std::isdigit(
                   static_cast<unsigned char>(body[position]))) {
            ++position;
        }

        if (position == digitsStart) {
            return false;
        }

        if (quoted &&
            (position >= body.size() || body[position] != '\"')) {
            return false;
        }

        const std::string number =
            body.substr(start, position - start);

        try {
            std::size_t parsed = 0;
            const long long converted = std::stoll(number, &parsed, 10);
            if (parsed != number.size()) {
                return false;
            }
            value = converted;
            return true;
        }
        catch (...) {
            return false;
        }
    }

    static bool parseStringArrayField(
        const std::string& body,
        const std::string& fieldName,
        std::vector<std::string>& values)
    {
        std::size_t position = 0;
        if (!findFieldValue(body, fieldName, position) ||
            body[position] != '[') {
            return false;
        }

        ++position;
        values.clear();

        while (position < body.size()) {
            skipWhitespace(body, position);
            if (position < body.size() && body[position] == ']') {
                return true;
            }

            std::string value;
            if (!parseStringValue(body, position, value)) {
                return false;
            }

            values.push_back(value);
            skipWhitespace(body, position);

            if (position >= body.size()) {
                return false;
            }

            if (body[position] == ']') {
                return true;
            }

            if (body[position] != ',') {
                return false;
            }

            ++position;
        }

        return false;
    }
};
