#pragma once

#include "RecordingActionExecutionResult.h"
#include "RecordingActionRequest.h"
#include "RecordingActionUtils.h"
#include "PublicResourcePreconditions.h"

#include <cctype>
#include <string>

struct PublicRecordingActionProviderPreflight final
{
    std::string recordingsState;
    std::string timersState;

    static bool field(const std::string& json, const std::string& key,
                      bool boolean, std::string& value)
    {
        if (json.size() > 65536U)
            return false;
        const std::string marker = "\"" + key + "\"";
        const auto pos = json.find(marker);
        if (pos == std::string::npos ||
            json.find(marker, pos + marker.size()) != std::string::npos)
            return false;
        std::size_t cursor = pos + marker.size();
        while (cursor < json.size() &&
               std::isspace(static_cast<unsigned char>(json[cursor])) != 0)
            ++cursor;
        if (cursor >= json.size() || json[cursor++] != ':')
            return false;
        while (cursor < json.size() &&
               std::isspace(static_cast<unsigned char>(json[cursor])) != 0)
            ++cursor;
        if (boolean)
        {
            if (json.compare(cursor, 4, "true") != 0)
                return false;
            cursor += 4;
            value = "true";
        }
        else
        {
            const bool quoted = cursor < json.size() && json[cursor] == '"';
            if (quoted)
                ++cursor;
            const std::size_t begin = cursor;
            while (cursor < json.size() &&
                   std::isdigit(static_cast<unsigned char>(json[cursor])) != 0 &&
                   cursor - begin < 20U)
                ++cursor;
            if (cursor == begin)
                return false;
            value = json.substr(begin, cursor - begin);
            if (quoted && (cursor >= json.size() || json[cursor++] != '"'))
                return false;
        }
        while (cursor < json.size() &&
               std::isspace(static_cast<unsigned char>(json[cursor])) != 0)
            ++cursor;
        return cursor < json.size() &&
            (json[cursor] == ',' || json[cursor] == '}');
    }

    static bool read(const RecordingActionExecutionResult& result,
                     PublicRecordingActionProviderPreflight& preview)
    {
        preview = {};
        if (result.upstreamHttpStatus < 200 ||
            result.upstreamHttpStatus >= 300 ||
            !result.errors.empty() ||
            result.upstreamEndpoint.find("/preview.json") ==
                std::string::npos)
            return false;
        std::string executable;
        return field(result.upstreamResponseBody, "executable", true, executable) &&
            field(result.upstreamResponseBody, "revision_recordings_state",
                  false, preview.recordingsState) &&
            field(result.upstreamResponseBody, "revision_timers_state",
                  false, preview.timersState);
    }

    // This ETag contains only public scope, intended operation and provider
    // revision counters. Never include backend-native paths or service IDs.
    static std::string tag(const RecordingActionRequest& publicRequest,
                           const std::string& publicId,
                           const PublicRecordingActionProviderPreflight& preview)
    {
        const auto append = [](std::string& dst, const std::string& value) {
            dst += std::to_string(value.size());
            dst += ':';
            dst += value;
        };
        std::string revision = "public-recording-action-preview/1:";
        append(revision, publicRequest.backendId);
        append(revision, publicId);
        append(revision, toString(publicRequest.type));
        auto name = publicRequest.parameters.find("newName");
        append(revision, name == publicRequest.parameters.end()
            ? "" : name->second);
        auto folder = publicRequest.parameters.find("publicTargetFolderId");
        append(revision, folder == publicRequest.parameters.end()
            ? "" : folder->second);
        append(revision, preview.recordingsState);
        append(revision, preview.timersState);
        return vdrsuite::http::publicStrongEntityTag(revision);
    }
};
