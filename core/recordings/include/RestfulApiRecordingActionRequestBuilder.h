#pragma once

#include "HttpRequest.h"
#include "RecordingAction.h"
#include "RecordingActionJobPayload.h"
#include "RestfulApiRecordingActionBackendConfig.h"

#include <algorithm>
#include <map>
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

        if (targetPath.empty() || leaf.empty()) {
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

    static std::string buildDeleteBody(
        const RecordingActionJobPayload& payload)
    {
        std::string body = "{";
        body += "\"file\":" + jsonQuote(recordingPath(payload));
        body += "}";

        return body;
    }
};
