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
inline ApiResponse render(const Snapshot& snapshot) {
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
    result.body += "]}";
    return result;
}
inline bool parseQuery(const std::string& uri, std::string& backend) {
    constexpr const char* prefix =
        "/api/v1/addons/rectools/sources?backendId=";
    if (uri.rfind(prefix, 0) != 0) return false;
    backend = uri.substr(std::strlen(prefix));
    return backendId(backend);
}
inline ApiResponse read(const std::string& uri) {
    std::string requested;
    if (!parseQuery(uri, requested)) return error(422, "invalid_backend");
    Snapshot current;
    if (!readLedger(current)) return error(503, "sources_unavailable");
    if (current.backend != requested)
        return error(409, "backend_is_not_local");
    return render(current);
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
                      std::string& expected, std::vector<Source>& sources) {
    if (body.size() > 32000U) return false;
    static const std::regex format(
        R"MT(^\{"backendId":"([A-Za-z0-9_.:-]{1,128})","revision":"([0-9a-f]{16})","entries":"([A-Za-z0-9%_.!~*'()-]{1,30000})"\}$)MT");
    std::smatch match;
    if (!std::regex_match(body, match, format)) return false;
    backend = match[1].str();
    expected = match[2].str();
    if (!backendId(backend)) return false;
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
inline ApiResponse save(const std::string& body) {
    std::string backend, expected;
    std::vector<Source> proposed;
    if (!parseBody(body, backend, expected, proposed))
        return error(422, "invalid_sources");
    if (::geteuid() != 0) return error(503, "sources_not_writable");
    if (!protectedConfigDirs() || !rootOwnedFile(Lock))
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
        if (!readLedger(current)) break;
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
        if (written) written = ::rename(temp, Ledger)==0;
        if (!written) { ::unlink(temp); break; }
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
