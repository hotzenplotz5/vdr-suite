#pragma once

#include "HttpRequest.h"
#include "RecordingAction.h"
#include "RecordingActionJobPayload.h"
#include "RestfulApiRecordingActionBackendConfig.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class RestfulApiRecordingActionRequestBuilder
{
public:
    HttpRequest buildMoveRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(config.basePath, "/recordings/move.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildMoveBody(payload);
        return request;
    }

    HttpRequest buildSafeMovePreviewRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/move/preview.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeMovePreviewBody(
                config,
                payload);
        return request;
    }

    HttpRequest buildSafeMoveValidateRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/move/validate.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeMoveRevisionBody(
                config,
                payload,
                recordingsState,
                timersState);
        return request;
    }

    HttpRequest buildSafeMoveExecuteRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/move.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeMoveRevisionBody(
                config,
                payload,
                recordingsState,
                timersState);
        return request;
    }

    HttpRequest buildSafeRenamePreviewRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/rename/preview.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeRenamePreviewBody(payload);
        return request;
    }

    HttpRequest buildSafeRenameValidateRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/rename/validate.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeRenameRevisionBody(
                payload,
                recordingsState,
                timersState);
        return request;
    }

    HttpRequest buildSafeRenameExecuteRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/rename.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeRenameRevisionBody(
                payload,
                recordingsState,
                timersState);
        return request;
    }

    HttpRequest buildRenameRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(config.basePath, "/recordings/move.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildRenameBody(payload);
        return request;
    }

    HttpRequest buildSafeTrashPreviewRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/trash/preview.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeTrashPreviewBody(payload);
        return request;
    }

    HttpRequest buildSafeTrashValidateRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/trash/validate.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeTrashRevisionBody(
                payload,
                recordingsState,
                timersState);
        return request;
    }

    HttpRequest buildSafeTrashExecuteRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url =
            buildUrl(
                config.basePath,
                "/recordings/trash.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body =
            buildSafeTrashRevisionBody(
                payload,
                recordingsState,
                timersState);
        return request;
    }

    HttpRequest buildDeleteRequest(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload) const
    {
        HttpRequest request;
        request.method = "POST";
        request.url = buildUrl(config.basePath, "/recordings/delete.json");
        request.headers["Accept"] = "application/json";
        request.headers["Content-Type"] = "application/json";
        request.body = buildDeleteBody(payload);
        return request;
    }

private:
    static std::string buildUrl(
        const std::string& basePath,
        const std::string& endpoint)
    {
        if (basePath.empty()) {
            return endpoint;
        }

        if (basePath.back() == '/' && endpoint.front() == '/') {
            return basePath.substr(0, basePath.size() - 1) + endpoint;
        }

        if (basePath.back() != '/' && endpoint.front() != '/') {
            return basePath + "/" + endpoint;
        }

        return basePath + endpoint;
    }

    static std::string findParameter(
        const std::map<std::string, std::string>& parameters,
        const std::string& name)
    {
        const auto it = parameters.find(name);

        if (it == parameters.end()) {
            return "";
        }

        return it->second;
    }

    static std::string recordingPath(
        const RecordingActionJobPayload& payload)
    {
        const std::string backendNativeId =
            findParameter(payload.parameters, "backendNativeId");

        if (!backendNativeId.empty()) {
            return backendNativeId;
        }

        const std::string path =
            findParameter(payload.parameters, "recordingPath");

        if (!path.empty()) {
            return path;
        }

        return payload.recordingId;
    }

    static std::string encodeVdrFolderTarget(
        const std::string& target)
    {
        std::string encoded;

        for (char c : target) {
            if (c == '/') {
                encoded += '~';
            } else {
                encoded += c;
            }
        }

        return encoded;
    }

    static std::string encodeVdrRecordingNameTarget(
        const std::string& target)
    {
        std::string encoded;

        for (char c : target) {
            if (c == '/') {
                encoded += '~';
            } else if (c == ' ') {
                encoded += '_';
            } else {
                encoded += c;
            }
        }

        return encoded;
    }

    static std::string recordingLeafName(
        const std::string& source)
    {
        const std::size_t recSeparator =
            source.find_last_of('/');

        if (recSeparator == std::string::npos || recSeparator == 0) {
            return "";
        }

        const std::size_t leafEnd =
            recSeparator - 1;

        const std::size_t leafStart =
            source.find_last_of('/', leafEnd);

        if (leafStart == std::string::npos) {
            return source.substr(0, leafEnd + 1);
        }

        return source.substr(leafStart + 1, leafEnd - leafStart);
    }

    static std::string normalizedRecordingPath(
        const RecordingActionJobPayload& payload)
    {
        const std::vector<std::string> segments =
            splitRecordingPathSegments(recordingPath(payload));

        if (segments.empty()) {
            return recordingPath(payload);
        }

        return "/" + joinRecordingPathSegments(segments);
    }

    static std::string recordingLogicalLeafName(
        const RecordingActionJobPayload& payload)
    {
        std::string logicalName =
            findParameter(payload.parameters, "recordingTitle");

        if (logicalName.empty()) {
            return recordingLeafName(
                normalizedRecordingPath(payload));
        }

        std::replace(
            logicalName.begin(),
            logicalName.end(),
            '~',
            '/');

        while (!logicalName.empty() &&
               logicalName.back() == '/') {
            logicalName.pop_back();
        }

        const std::size_t separator =
            logicalName.find_last_of('/');

        if (separator == std::string::npos) {
            return logicalName;
        }

        return logicalName.substr(separator + 1);
    }

    static std::string moveTarget(
        const std::string& targetPath,
        const RecordingActionJobPayload& payload)
    {
        const std::string leaf =
            recordingLogicalLeafName(payload);

        if (leaf.empty()) {
            return targetPath;
        }

        if (targetPath == "/") {
            return leaf;
        }

        if (targetPath.empty()) {
            return targetPath;
        }

        if (targetPath.back() == '/') {
            return targetPath + leaf;
        }

        return targetPath + "/" + leaf;
    }

    static bool isStorageMountSegment(const std::string& segment)
    {
        return segment == "Recordings_on_yavdr(nfs)";
    }

    static std::vector<std::string> splitRecordingPathSegments(
        std::string source)
    {
        const std::string srvPrefix =
            "/srv/vdr/video/";

        if (source.rfind(srvPrefix, 0) == 0) {
            source = source.substr(srvPrefix.size());
        }

        while (!source.empty() && source.front() == '/') {
            source.erase(source.begin());
        }

        while (!source.empty() && source.back() == '/') {
            source.pop_back();
        }

        std::vector<std::string> segments;
        std::string current;

        for (char c : source) {
            if (c == '/') {
                if (!current.empty() && !isStorageMountSegment(current)) {
                    segments.push_back(current);
                }

                current.clear();
                continue;
            }

            current.push_back(c);
        }

        if (!current.empty() && !isStorageMountSegment(current)) {
            segments.push_back(current);
        }

        return segments;
    }

    static std::string joinRecordingPathSegments(
        const std::vector<std::string>& segments)
    {
        std::string result;

        for (const std::string& segment : segments) {
            if (!result.empty()) {
                result += "/";
            }

            result += segment;
        }

        return result;
    }

    static std::string recordingParentFolder(
        const RecordingActionJobPayload& payload)
    {
        std::vector<std::string> segments =
            splitRecordingPathSegments(recordingPath(payload));

        if (segments.size() <= 2) {
            return "";
        }

        segments.pop_back();
        segments.pop_back();

        return joinRecordingPathSegments(segments);
    }

    static std::string renameTarget(
        const RecordingActionJobPayload& payload,
        const std::string& newName)
    {
        if (newName.find('/') != std::string::npos ||
            newName.find('~') != std::string::npos) {
            return newName;
        }

        const std::string parent =
            recordingParentFolder(payload);

        if (parent.empty()) {
            return newName;
        }

        return parent + "/" + newName;
    }

    static std::string normalizeSlashes(
        std::string value)
    {
        std::replace(
            value.begin(),
            value.end(),
            '\\',
            '/');

        while (value.size() > 1 &&
               value.back() == '/')
        {
            value.pop_back();
        }

        return value;
    }

    static std::string lastPathSegment(
        const std::string& path)
    {
        const std::string normalized =
            normalizeSlashes(path);

        const std::size_t separator =
            normalized.find_last_of('/');

        if (separator == std::string::npos)
        {
            return normalized;
        }

        return normalized.substr(separator + 1);
    }

    static std::string parentPath(
        const std::string& path)
    {
        const std::string normalized =
            normalizeSlashes(path);

        const std::size_t separator =
            normalized.find_last_of('/');

        if (separator == std::string::npos)
        {
            return "";
        }

        if (separator == 0)
        {
            return "/";
        }

        return normalized.substr(0, separator);
    }

    static bool endsWith(
        const std::string& value,
        const std::string& suffix)
    {
        return
            suffix.size() <= value.size() &&
            value.compare(
                value.size() - suffix.size(),
                suffix.size(),
                suffix) == 0;
    }

    static bool isHexDigit(
        char value)
    {
        return std::isxdigit(
            static_cast<unsigned char>(value)) != 0;
    }

    static bool isVdrFilesystemEscape(
        const std::string& value,
        std::size_t index)
    {
        return
            value[index] == '#' &&
            index + 2 < value.size() &&
            isHexDigit(value[index + 1]) &&
            isHexDigit(value[index + 2]);
    }

    static bool requiresVdrFilesystemEscape(
        unsigned char value)
    {
        const std::string invalid =
            "\"'\\/:*?|<>#~";

        return invalid.find(
            static_cast<char>(value)) !=
            std::string::npos;
    }

    static std::string encodeVdrFilesystemSegment(
        const std::string& segment)
    {
        std::ostringstream encoded;

        for (std::size_t index = 0;
             index < segment.size();
             ++index)
        {
            const unsigned char value =
                static_cast<unsigned char>(
                    segment[index]);

            if (isVdrFilesystemEscape(
                    segment,
                    index))
            {
                encoded <<
                    segment.substr(index, 3);
                index += 2;
                continue;
            }

            if (value == ' ')
            {
                encoded << '_';
                continue;
            }

            if (requiresVdrFilesystemEscape(value))
            {
                encoded
                    << '#'
                    << std::uppercase
                    << std::hex
                    << std::setw(2)
                    << std::setfill('0')
                    << static_cast<int>(value)
                    << std::dec;
                continue;
            }

            encoded <<
                static_cast<char>(value);
        }

        return encoded.str();
    }

    static std::string encodeVdrFilesystemPath(
        const std::string& path)
    {
        const std::vector<std::string> segments =
            splitRecordingPathSegments(path);

        std::vector<std::string> encodedSegments;
        encodedSegments.reserve(segments.size());

        for (const std::string& segment : segments)
        {
            encodedSegments.push_back(
                encodeVdrFilesystemSegment(segment));
        }

        return joinRecordingPathSegments(
            encodedSegments);
    }

    static std::string recordingVideoDirectory(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload)
    {
        const std::string source =
            normalizeSlashes(
                recordingPath(payload));

        std::string relative =
            normalizeSlashes(
                findParameter(
                    payload.parameters,
                    "recordingPath"));

        if (!relative.empty() &&
            relative.front() != '/')
        {
            relative = "/" + relative;
        }

        if (!relative.empty() &&
            endsWith(source, relative))
        {
            std::string directory =
                source.substr(
                    0,
                    source.size() - relative.size());

            while (directory.size() > 1 &&
                   directory.back() == '/')
            {
                directory.pop_back();
            }

            if (!directory.empty())
            {
                return directory;
            }
        }

        std::string directory =
            normalizeSlashes(
                config.videoDirectory);

        if (directory.empty())
        {
            directory = "/srv/vdr/video";
        }

        return directory;
    }

    static std::string safeMoveTargetFile(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload)
    {
        const std::string source =
            normalizeSlashes(
                recordingPath(payload));

        const std::string timestampDirectory =
            lastPathSegment(source);

        const std::string nativeTitleDirectory =
            lastPathSegment(
                parentPath(source));

        const std::string targetPath =
            findParameter(
                payload.parameters,
                "targetPath");

        const std::string encodedTargetPath =
            targetPath == "/"
                ? ""
                : encodeVdrFilesystemPath(
                    targetPath);

        std::string target =
            recordingVideoDirectory(
                config,
                payload);

        if (!encodedTargetPath.empty())
        {
            target += "/" + encodedTargetPath;
        }

        if (!nativeTitleDirectory.empty())
        {
            target += "/" + nativeTitleDirectory;
        }

        if (!timestampDirectory.empty())
        {
            target += "/" + timestampDirectory;
        }

        return normalizeSlashes(target);
    }

    static std::string jsonQuote(const std::string& value)
    {
        std::string quoted = "\"";

        for (char c : value) {
            if (c == '"' || c == '\\') {
                quoted += '\\';
            }

            quoted += c;
        }

        quoted += "\"";
        return quoted;
    }

    static std::string buildMoveBody(
        const RecordingActionJobPayload& payload)
    {
        const std::string targetPath =
            findParameter(payload.parameters, "targetPath");

        std::string body = "{";
        body += "\"source\":" + jsonQuote(recordingPath(payload));
        body += ",\"target\":" + jsonQuote(encodeVdrFolderTarget(moveTarget(targetPath, payload)));
        body += ",\"copy_only\":false";
        body += "}";

        return body;
    }

    static std::string buildSafeMovePreviewBody(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload)
    {
        std::string body = "{";

        body +=
            "\"file\":" +
            jsonQuote(recordingPath(payload));

        body +=
            ",\"target_file\":" +
            jsonQuote(
                safeMoveTargetFile(
                    config,
                    payload));

        body += "}";

        return body;
    }

    static std::string buildSafeMoveRevisionBody(
        const RestfulApiRecordingActionBackendConfig& config,
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState)
    {
        std::string body =
            buildSafeMovePreviewBody(
                config,
                payload);

        body.pop_back();

        body +=
            ",\"revision_recordings_state\":" +
            jsonQuote(recordingsState);

        body +=
            ",\"revision_timers_state\":" +
            jsonQuote(timersState);

        body += "}";

        return body;
    }

    static std::string safeRenameName(
        const RecordingActionJobPayload& payload)
    {
        return encodeVdrFilesystemSegment(
            findParameter(
                payload.parameters,
                "newName"));
    }

    static std::string buildSafeRenamePreviewBody(
        const RecordingActionJobPayload& payload)
    {
        std::string body = "{";

        body +=
            "\"file\":" +
            jsonQuote(recordingPath(payload));

        body +=
            ",\"name\":" +
            jsonQuote(
                safeRenameName(payload));

        body += "}";

        return body;
    }

    static std::string buildSafeRenameRevisionBody(
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState)
    {
        std::string body =
            buildSafeRenamePreviewBody(payload);

        body.pop_back();

        body +=
            ",\"revision_recordings_state\":" +
            jsonQuote(recordingsState);

        body +=
            ",\"revision_timers_state\":" +
            jsonQuote(timersState);

        body += "}";

        return body;
    }

    static std::string buildRenameBody(
        const RecordingActionJobPayload& payload)
    {
        const std::string newName =
            findParameter(payload.parameters, "newName");

        std::string body = "{";
        body += "\"source\":" + jsonQuote(recordingPath(payload));
        body += ",\"target\":" + jsonQuote(encodeVdrRecordingNameTarget(renameTarget(payload, newName)));
        body += ",\"copy_only\":false";
        body += "}";

        return body;
    }

    static std::string buildSafeTrashPreviewBody(
        const RecordingActionJobPayload& payload)
    {
        std::string body = "{";

        body +=
            "\"file\":" +
            jsonQuote(recordingPath(payload));

        body += "}";

        return body;
    }

    static std::string buildSafeTrashRevisionBody(
        const RecordingActionJobPayload& payload,
        const std::string& recordingsState,
        const std::string& timersState)
    {
        std::string body =
            buildSafeTrashPreviewBody(payload);

        body.pop_back();

        body +=
            ",\"revision_recordings_state\":" +
            jsonQuote(recordingsState);

        body +=
            ",\"revision_timers_state\":" +
            jsonQuote(timersState);

        body += "}";

        return body;
    }

    static std::string buildDeleteBody(
        const RecordingActionJobPayload& payload)
    {
        std::string body = "{";
        body += "\"file\":" + jsonQuote(recordingPath(payload));
        body += "}";

        return body;
    }
};
