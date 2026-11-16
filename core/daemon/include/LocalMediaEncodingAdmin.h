#pragma once
// MT.ENC3.2: the existing backend-agent.conf is the only settings store.
#include "LocalMediaImportSourceAdmin.h"
#include "MediaImport.h"
#include <array>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <sys/stat.h>

namespace vdrsuite::mediatools::local_encoding_admin {
constexpr const char* File = "/etc/vdr-suite/backend-agent.conf";
struct Snapshot {
    std::string backend;
    std::string revision;
    MediaEncodingSettings settings;
};
inline bool snapshot(Snapshot& result, std::string& raw, struct stat* metadata = nullptr) {
    result = {};
    raw.clear();
    if (!local_source_admin::rootOwnedFile(File)) return false;
    const int fd = ::open(File, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return false;
    struct stat info{};
    bool ok = ::fstat(fd, &info) == 0 && S_ISREG(info.st_mode) &&
        info.st_uid == 0 && info.st_nlink == 1 &&
        (info.st_mode & 0022) == 0 && info.st_size >= 0 && info.st_size <= 16384;
    if (ok) {
        std::array<char, 4096> bytes{};
        ssize_t n = 0;
        while ((n = ::read(fd, bytes.data(), bytes.size())) > 0) {
            raw.append(bytes.data(), static_cast<std::size_t>(n));
            if (raw.size() > 16384U) { ok = false; break; }
        }
        if (n < 0) ok = false;
    }
    ::close(fd);
    if (!ok) return false;
    std::map<std::string, std::string> values;
    std::istringstream input(raw);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        const auto first = key.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        key = key.substr(first, key.find_last_not_of(" \t") - first + 1);
        if (key == "BACKEND_ID" ||
            key == "MEDIA_IMPORT_H264_CRF" ||
            key == "MEDIA_IMPORT_H264_PRESET" ||
            key == "MEDIA_SHRINK_H265_CRF" ||
            key == "MEDIA_SHRINK_H265_PRESET")
            if (!values.emplace(key, line.substr(eq + 1)).second) return false;
    }
    if (!local_source_admin::backendId(values["BACKEND_ID"])) return false;
    result.backend = values["BACKEND_ID"];
    auto parseNumber = [&](const char* name, int& destination) {
        const auto at = values.find(name);
        if (at == values.end()) return true;
        const auto& value = at->second;
        if (value.empty() || value.size() > 2 ||
            !std::all_of(value.begin(), value.end(), [](unsigned char c) {
                return c >= '0' && c <= '9';
            })) return false;
        destination = std::stoi(value);
        return destination <= 51;
    };
    if (!parseNumber("MEDIA_IMPORT_H264_CRF", result.settings.importH264Crf) ||
        !parseNumber("MEDIA_SHRINK_H265_CRF", result.settings.shrinkH265Crf))
        return false;
    if (values.count("MEDIA_IMPORT_H264_PRESET"))
        result.settings.importH264Preset = values["MEDIA_IMPORT_H264_PRESET"];
    if (values.count("MEDIA_SHRINK_H265_PRESET"))
        result.settings.shrinkH265Preset = values["MEDIA_SHRINK_H265_PRESET"];
    if (!result.settings.valid()) return false;
    result.revision = local_source_admin::revision(raw);
    if (metadata) *metadata = info;
    return true;
}
inline ApiResponse reply(const Snapshot& s) {
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    std::ostringstream json;
    json << "{\"backendId\":\"" << s.backend
         << "\",\"revision\":\"" << s.revision
         << "\",\"importCrf\":" << s.settings.importH264Crf
         << ",\"importPreset\":\"" << s.settings.importH264Preset
         << "\",\"shrinkCrf\":" << s.settings.shrinkH265Crf
         << ",\"shrinkPreset\":\"" << s.settings.shrinkH265Preset
         << "\",\"applies\":\"next_job\"}";
    response.body = json.str();
    return response;
}
inline ApiResponse read(const std::string& uri) {
    const std::string prefix = "/api/v1/addons/media-tools/encoding?backendId=";
    if (uri.rfind(prefix, 0) != 0)
        return local_source_admin::error(422, "invalid_backend");
    std::string requested;
    if (!local_source_admin::decode(uri.substr(prefix.size()), requested) ||
        !local_source_admin::backendId(requested))
        return local_source_admin::error(422, "invalid_backend");
    Snapshot current;
    std::string raw;
    if (!snapshot(current, raw))
        return local_source_admin::error(503, "encoding_unavailable");
    if (current.backend != requested)
        return local_source_admin::error(409, "backend_is_not_local");
    return reply(current);
}
inline bool parse(const std::string& body, std::string& backend,
                  std::string& revision, MediaEncodingSettings& settings) {
    if (body.size() > 512U) return false;
    static const std::regex schema(
        R"ENC(^\{"backendId":"([A-Za-z0-9_.:-]{1,128})","revision":"([0-9a-f]{16})","importCrf":([0-9]{1,2}),"importPreset":"([a-z]{3,12})","shrinkCrf":([0-9]{1,2}),"shrinkPreset":"([a-z]{3,12})"\}$)ENC");
    std::smatch match;
    if (!std::regex_match(body, match, schema)) return false;
    backend = match[1].str();
    revision = match[2].str();
    settings.importH264Crf = std::stoi(match[3].str());
    settings.importH264Preset = match[4].str();
    settings.shrinkH265Crf = std::stoi(match[5].str());
    settings.shrinkH265Preset = match[6].str();
    return local_source_admin::backendId(backend) && settings.valid();
}
inline std::string rewrite(const std::string& original,
                           const MediaEncodingSettings& settings) {
    const std::map<std::string, std::string> replacements = {
        {"MEDIA_IMPORT_H264_CRF", std::to_string(settings.importH264Crf)},
        {"MEDIA_IMPORT_H264_PRESET", settings.importH264Preset},
        {"MEDIA_SHRINK_H265_CRF", std::to_string(settings.shrinkH265Crf)},
        {"MEDIA_SHRINK_H265_PRESET", settings.shrinkH265Preset}
    };
    std::string output;
    std::set<std::string> found;
    std::istringstream lines(original);
    std::string line;
    while (std::getline(lines, line)) {
        const auto eq = line.find('=');
        const auto key = eq == std::string::npos ? line : line.substr(0, eq);
        const auto at = replacements.find(key);
        if (at != replacements.end()) {
            output += at->first + "=" + at->second + "\n";
            found.insert(at->first);
        } else output += line + "\n";
    }
    for (const auto& item : replacements)
        if (!found.count(item.first))
            output += item.first + "=" + item.second + "\n";
    return output;
}
inline ApiResponse save(const std::string& body) {
    std::string backend, revision;
    MediaEncodingSettings selected;
    if (!parse(body, backend, revision, selected))
        return local_source_admin::error(422, "invalid_encoding_settings");
    if (::geteuid() != 0)
        return local_source_admin::error(503, "encoding_not_writable");
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);
    Snapshot current;
    std::string original;
    struct stat info{};
    if (!snapshot(current, original, &info))
        return local_source_admin::error(503, "encoding_unavailable");
    if (current.backend != backend)
        return local_source_admin::error(409, "backend_is_not_local");
    if (current.revision != revision)
        return local_source_admin::error(409, "encoding_changed");
    const auto replacement = rewrite(original, selected);
    if (replacement == original) return reply(current);
    if (replacement.size() > 16384U)
        return local_source_admin::error(422, "invalid_encoding_settings");
    char temp[] = "/etc/vdr-suite/.backend-agent-encoding-XXXXXX";
    const int fd = ::mkstemp(temp);
    if (fd < 0) return local_source_admin::error(503, "encoding_write_failed");
    bool ok = ::fchown(fd, info.st_uid, info.st_gid) == 0 &&
              ::fchmod(fd, info.st_mode & 0777) == 0;
    std::size_t offset = 0;
    while (ok && offset < replacement.size()) {
        const ssize_t n = ::write(fd, replacement.data() + offset,
                                  replacement.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { ok = false; break; }
        offset += static_cast<std::size_t>(n);
    }
    if (ok) ok = ::fsync(fd) == 0;
    if (::close(fd) != 0) ok = false;
    if (ok) ok = ::rename(temp, File) == 0;
    if (!ok) {
        ::unlink(temp);
        return local_source_admin::error(503, "encoding_write_failed");
    }
    const int dir = ::open("/etc/vdr-suite",
                           O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dir >= 0) { (void)::fsync(dir); ::close(dir); }
    Snapshot verified;
    std::string text;
    if (!snapshot(verified, text) || verified.backend != backend ||
        verified.settings.importH264Crf != selected.importH264Crf ||
        verified.settings.importH264Preset != selected.importH264Preset ||
        verified.settings.shrinkH265Crf != selected.shrinkH265Crf ||
        verified.settings.shrinkH265Preset != selected.shrinkH265Preset)
        return local_source_admin::error(503, "encoding_readback_failed");
    return reply(verified);
}
} // namespace vdrsuite::mediatools::local_encoding_admin
