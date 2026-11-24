#pragma once

// Local Media Tools source settings. Reuses the existing root-owned inventory
// and the daemon's already-authenticated admin route. No helper process,
// duplicate grants, service restart or alternative importer.
#include "DashboardController.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <initializer_list>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace vdrsuite::mediatools {
namespace local_source_admin {
constexpr const char* Ledger = "/etc/vdr-suite/addon-sources/media-import.tsv";
constexpr const char* Lock = "/etc/vdr-suite/addon-approvals/rectools.import-running.lock";

struct Source {
    std::string path;
    std::string name;
};
struct Snapshot {
    std::string backend;
    std::string videoRoot;
    std::string revision;
    std::vector<Source> sources;
};

inline ApiResponse error(int status, const char* code) {
    ApiResponse result;
    result.statusCode = status;
    result.contentType = "application/json";
    result.headers["Cache-Control"] = "no-store";
    result.body = std::string("{\"error\":\"") + code + "\"}";
    return result;
}
inline std::string escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c >= 0x20U) out += static_cast<char>(c);
    }
    return out;
}
inline bool backendId(const std::string& s) {
    return !s.empty() && s.size() <= 128U && s != "." && s != ".." &&
        s != "*" && std::all_of(s.begin(), s.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '-' ||
                c == '.' || c == ':';
        });
}
inline bool sourceName(const std::string& name) {
    return !name.empty() && name.size() <= 64U &&
        std::none_of(name.begin(), name.end(), [](unsigned char c) {
            return c < 0x20U || c == 0x7fU || c == '/' ||
                c == '\\' || c == '"';
        });
}
inline bool rootSyntax(const std::string& value) {
    namespace fs = std::filesystem;
    if (value.empty() || value.size() > 512U || value[0] != '/' ||
        value == "/" || value.find('\\') != std::string::npos ||
        std::any_of(value.begin(), value.end(), [](unsigned char c) {
            return c < 0x20U || c == 0x7fU;
        })) return false;
    if (value.rfind("/home/", 0) == 0 || value == "/home" ||
        value.rfind("/root/", 0) == 0 || value == "/root" ||
        value.rfind("/run/user/", 0) == 0) return false;
    const fs::path p(value);
    return p == p.lexically_normal() && !value.empty() &&
        value.back() != '/';
}
inline bool safeRoot(const std::string& value) {
    namespace fs = std::filesystem;
    if (!rootSyntax(value)) return false;
    try {
        const fs::path p(value);
        return fs::is_directory(p) && fs::canonical(p) == p;
    } catch (...) { return false; }
}
inline bool overlap(const std::string& a, const std::string& b) {
    return a == b || a.compare(0, b.size()+1, b+"/") == 0 ||
        b.compare(0, a.size()+1, a+"/") == 0;
}
inline bool protectedConfigDirs() {
    for (const char* path : {"/etc/vdr-suite",
                              "/etc/vdr-suite/addon-sources",
                              "/etc/vdr-suite/addon-approvals"}) {
        struct stat st{};
        if (::lstat(path, &st) != 0 || !S_ISDIR(st.st_mode) ||
            st.st_uid != 0 || (st.st_mode & 0022) != 0)
            return false;
    }
    return true;
}
inline bool rootOwnedFile(const char* path) {
    struct stat st{};
    return ::lstat(path, &st) == 0 && S_ISREG(st.st_mode) &&
        st.st_uid == 0 && st.st_nlink == 1 &&
        (st.st_mode & 0022) == 0 && st.st_size >= 0 && st.st_size < 32768;
}
inline bool localBackend(std::string& id) {
    id.clear();
    std::ifstream stream("/etc/vdr-suite/backend-agent.conf");
    if (!stream) return false;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.rfind("BACKEND_ID=", 0) == 0) {
            if (!id.empty()) return false;
            id = line.substr(11);
        }
    }
    return backendId(id);
}
inline std::string revision(const std::string& contents) {
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : contents) {
        hash ^= c;
        hash *= UINT64_C(1099511628211);
    }
    std::ostringstream result;
    result << std::hex << std::setw(16) << std::setfill('0') << hash;
    return result.str();
}
inline bool readLedger(Snapshot& out) {
    out = {};
    if (!protectedConfigDirs() || !rootOwnedFile(Ledger)) return false;
    const int fd = ::open(Ledger, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return false;
    struct stat st{};
    bool ok = ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
        st.st_uid == 0 && st.st_nlink == 1 &&
        (st.st_mode & 0022) == 0 && st.st_size < 32768;
    std::string content;
    if (ok) {
        std::array<char, 4096> buffer{};
        ssize_t n = 0;
        while ((n = ::read(fd, buffer.data(), buffer.size())) > 0) {
            content.append(buffer.data(), static_cast<std::size_t>(n));
            if (content.size() > 32768U) { ok = false; break; }
        }
        if (n < 0) ok = false;
    }
    ::close(fd);
    if (!ok || content.empty() || content.back() != '\n') return false;
    std::string local;
    if (!localBackend(local)) return false;
    std::istringstream input(content);
    std::string line;
    unsigned index = 0;
    while (std::getline(input, line)) {
        std::vector<std::string> col;
        std::size_t off = 0;
        for (;;) {
            const auto end = line.find('\t', off);
            col.push_back(line.substr(off, end == std::string::npos
                ? std::string::npos : end-off));
            if (end == std::string::npos) break;
            off = end + 1;
        }
        if (col.size() < 5U || col.size() > 6U ||
            col[0] != local || col[4] != "*" || !rootSyntax(col[2]) ||
            !rootSyntax(col[3]) || !sourceName(
                col.size() == 6U ? col[5] :
                std::filesystem::path(col[2]).filename().string()))
            return false;
        const std::string expected = index == 0U
            ? "local-media-import" :
            "local-media-root-" + [&]() {
                std::ostringstream text;
                text << std::setw(4) << std::setfill('0') << index;
                return text.str();
            }();
        if (col[1] != expected) return false;
        if (index == 0U) out.videoRoot = col[3];
        else if (col[3] != out.videoRoot) return false;
        out.sources.push_back({col[2], col.size() == 6U ? col[5] :
            std::filesystem::path(col[2]).filename().string()});
        if (++index > 17U) return false;
    }
    if (out.sources.empty()) return false;
    out.backend = local;
    out.revision = revision(content);
    return true;
}
inline ApiResponse render(const Snapshot& snapshot, bool initialSetup = false) {
    ApiResponse result;
    result.statusCode = 200;
    result.contentType = "application/json";
    result.headers["Cache-Control"] = "no-store";
    result.body = "{\"backendId\":\"" + escape(snapshot.backend) +
        "\",\"videoRoot\":\"" + escape(snapshot.videoRoot) +
        "\",\"revision\":\"" + snapshot.revision + "\",\"sources\":[";
    for (std::size_t i=0; i<snapshot.sources.size(); ++i) {
        if (i) result.body += ",";
        result.body += "{\"path\":\"" + escape(snapshot.sources[i].path) +
            "\",\"name\":\"" + escape(snapshot.sources[i].name) + "\"}";
    }
    result.body += "],\"initialSetup\":";
    result.body += initialSetup ? "true}" : "false}";
    return result;
}
inline bool parseQuery(const std::string& uri, std::string& backend) {
    constexpr const char* prefix =
        "/api/v1/addons/rectools/sources?backendId=";
    if (uri.rfind(prefix, 0) != 0) return false;
    backend = uri.substr(std::strlen(prefix));
    return backendId(backend);
}
// Missing is a first-run state, never equivalent to an invalid or unsafe
// existing inventory. No config file is created by a read request.
inline bool inventoryAbsent() {
    struct stat st{};
    return ::lstat(Ledger, &st) != 0 && errno == ENOENT;
}
inline bool initialSnapshot(Snapshot& out, const std::string& trustedVideoRoot) {
    if (!protectedConfigDirs() || !inventoryAbsent()) return false;
    std::string local;
    if (!localBackend(local)) return false;
    out = {};
    out.backend = local;
    // A missing recording-root discovery is visible, but cannot be used as a
    // destination. The actual VDR-owned root is provided by the daemon.
    out.videoRoot = rootSyntax(trustedVideoRoot) ? trustedVideoRoot : "";
    out.revision = revision("");
    return true;
}
inline ApiResponse read(const std::string& uri,
                        const std::string& trustedVideoRoot = {}) {
    std::string requested;
    if (!parseQuery(uri, requested)) return error(422, "invalid_backend");
    Snapshot current;
    bool initial = false;
    if (!readLedger(current)) {
        if (!initialSnapshot(current, trustedVideoRoot))
            return error(503, "sources_unavailable");
        initial = true;
    }
    if (current.backend != requested)
        return error(409, "backend_is_not_local");
    return render(current, initial);
}
inline bool decode(const std::string& encoded, std::string& decoded) {
    decoded.clear();
    for (std::size_t i=0; i<encoded.size(); ++i) {
        if (encoded[i] == '%') {
            if (i + 2 >= encoded.size()) return false;
            const auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            const int a = hex(encoded[++i]), b = hex(encoded[++i]);
            if (a < 0 || b < 0) return false;
            decoded += static_cast<char>(a*16 + b);
        } else decoded += encoded[i];
        if (decoded.size() > 9000U) return false;
    }
    return true;
}
inline bool parseBody(const std::string& body, std::string& backend,
                      std::string& expected, std::vector<Source>& sources,
                      std::string* requestedVideoRoot = nullptr) {
    if (requestedVideoRoot) requestedVideoRoot->clear();
    if (body.size() > 32000U) return false;
    static const std::regex format(
        R"MT(^\{"backendId":"([A-Za-z0-9_.:-]{1,128})","revision":"([0-9a-f]{16})","entries":"([A-Za-z0-9%_.!~*'()-]{1,30000})"(?:,"videoRoot":"([A-Za-z0-9%_.!~*'()-]{1,1536})")?\}$)MT");
    std::smatch match;
    if (!std::regex_match(body, match, format)) return false;
    backend = match[1].str();
    expected = match[2].str();
    if (!backendId(backend)) return false;
    // Explicit destination is accepted only for the initial setup without
    // an authoritative local VDR root. It is not an editable normal setting.
    if (match[4].matched) {
        if (!requestedVideoRoot ||
            !decode(match[4].str(), *requestedVideoRoot) ||
            !rootSyntax(*requestedVideoRoot)) return false;
    }
    std::string rows;
    if (!decode(match[3].str(), rows)) return false;
    std::istringstream input(rows);
    std::string line;
    sources.clear();
    while (std::getline(input, line)) {
        const auto tab = line.find('\t');
        if (tab == std::string::npos || line.find('\t', tab+1) != std::string::npos)
            return false;
        const Source source{line.substr(0,tab),line.substr(tab+1)};
        if (!safeRoot(source.path) || !sourceName(source.name)) return false;
        sources.push_back(source);
        if (sources.size() > 17U) return false;
    }
    return !sources.empty();
}
inline ApiResponse save(const std::string& body,
                        const std::string& trustedVideoRoot = {}) {
    std::string backend, expected, requestedVideoRoot;
    std::vector<Source> proposed;
    if (!parseBody(body, backend, expected, proposed, &requestedVideoRoot))
        return error(422, "invalid_sources");
    if (::geteuid() != 0) return error(503, "sources_not_writable");
    if (!protectedConfigDirs())
        return error(503, "source_lock_unavailable");
    // Only an authenticated first save may prepare the existing import
    // lifetime lock if the package deliberately did not ship host state.
    Snapshot before;
    bool initial = false;
    if (!readLedger(before)) {
        if (!initialSnapshot(before, trustedVideoRoot))
            return error(503, "sources_unavailable");
        initial = true;
    }
    if (before.backend != backend)
        return error(409, "backend_is_not_local");
    if (before.revision != expected)
        return error(409, "sources_changed");
    if (initial && before.videoRoot.empty()) {
        if (!safeRoot(requestedVideoRoot))
            return error(422, "invalid_video_root");
        before.videoRoot = requestedVideoRoot;
    } else if (!requestedVideoRoot.empty()) {
        // An HTTP payload may not override an existing or VDR-discovered root.
        return error(409, "video_root_already_known");
    }
    if (initial && !safeRoot(before.videoRoot))
        return error(503, "video_root_unavailable");
    if (initial && inventoryAbsent() && !rootOwnedFile(Lock)) {
        struct stat st{};
        if (::lstat(Lock, &st) != 0 && errno == ENOENT) {
            const int fd = ::open(Lock, O_WRONLY | O_CREAT | O_EXCL |
                O_NOFOLLOW | O_CLOEXEC, 0644);
            if (fd >= 0) {
                const bool valid = ::fchown(fd, 0, 0) == 0 &&
                    ::fchmod(fd, 0644) == 0 && ::fsync(fd) == 0;
                ::close(fd);
                if (!valid) return error(503, "source_lock_unavailable");
            }
        }
    }
    if (!rootOwnedFile(Lock))
        return error(503, "source_lock_unavailable");
    const int lock = ::open(Lock, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (lock < 0) return error(503, "source_lock_unavailable");
    if (::flock(lock, LOCK_EX | LOCK_NB) != 0) {
        ::close(lock);
        return error(409, "import_in_progress");
    }
    ApiResponse result = error(503, "sources_unavailable");
    Snapshot current;
    do {
        const bool creating = !readLedger(current);
        if (creating && !initialSnapshot(current, trustedVideoRoot)) break;
        if (creating && current.videoRoot.empty())
            current.videoRoot = requestedVideoRoot;
        if (creating && !safeRoot(current.videoRoot)) {
            result = error(503, "video_root_unavailable"); break;
        }
        if (current.backend != backend) {
            result = error(409, "backend_is_not_local"); break;
        }
        if (current.revision != expected) {
            result = error(409, "sources_changed"); break;
        }
        bool valid = true;
        for (std::size_t i=0; i<proposed.size(); ++i) {
            if (overlap(proposed[i].path, current.videoRoot)) valid = false;
            for (std::size_t j=0; j<i; ++j)
                if (overlap(proposed[i].path, proposed[j].path)) valid = false;
        }
        if (!valid) { result = error(422, "overlapping_roots"); break; }
        std::string output;
        for (std::size_t i=0; i<proposed.size(); ++i) {
            std::ostringstream id;
            if (i==0) id << "local-media-import";
            else id << "local-media-root-" << std::setw(4)
                    << std::setfill('0') << i;
            output += backend + "\t" + id.str() + "\t" +
                proposed[i].path + "\t" + current.videoRoot +
                "\t*\t" + proposed[i].name + "\n";
        }
        if (output == "") { result = error(422, "empty_sources"); break; }
        char temp[] = "/etc/vdr-suite/addon-sources/.media-import-XXXXXXXX";
        const int fd = ::mkstemp(temp);
        if (fd < 0) break;
        bool written = ::fchmod(fd, 0644)==0 && ::fchown(fd,0,0)==0;
        std::size_t offset=0;
        while (written && offset < output.size()) {
            const auto n = ::write(fd, output.data()+offset,
                output.size()-offset);
            if (n <= 0) { written = false; break; }
            offset += static_cast<std::size_t>(n);
        }
        if (written) written = ::fsync(fd)==0;
        ::close(fd);
        // An initial save must not overwrite a config created elsewhere.
        if (written) written = creating
            ? ::link(temp, Ledger)==0 : ::rename(temp, Ledger)==0;
        const int linkError = errno;
        if (written && creating) ::unlink(temp);
        if (!written) {
            ::unlink(temp);
            if (creating && linkError == EEXIST)
                result = error(409, "sources_changed");
            break;
        }
        const int directory = ::open("/etc/vdr-suite/addon-sources",
            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (directory < 0) break;
        const bool durable = ::fsync(directory) == 0;
        ::close(directory);
        if (!durable) break;
        // The Agent reads the atomically replaced inventory at its normal
        // heartbeat. No restart, service override or other write is needed.
        Snapshot changed;
        if (readLedger(changed) && changed.backend == backend &&
            changed.sources.size()==proposed.size()) result=render(changed);
    } while (false);
    ::flock(lock, LOCK_UN);
    ::close(lock);
    return result;
}
} // namespace local_source_admin
} // namespace vdrsuite::mediatools
