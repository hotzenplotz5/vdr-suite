#include "RestfulApiStatusMapper.h"

#include <cctype>
#include <charconv>
#include <string_view>
#include <string>

namespace {

std::size_t skipWhitespace(const std::string& text, std::size_t pos)
{
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
    return pos;
}

bool looksLikeJsonObject(const std::string& text)
{
    std::size_t start = skipWhitespace(text, 0);
    if (start >= text.size() || text[start] != '{') {
        return false;
    }

    bool inString = false;
    bool escaped = false;
    int depth = 0;

    for (std::size_t i = start; i < text.size(); ++i) {
        char c = text[i];

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            continue;
        }

        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                return skipWhitespace(text, i + 1) == text.size();
            }
        }
    }

    return false;
}

 // Find a direct member of the requested JSON object, never a similarly
 // named member inside another object, array or escaped string.
std::size_t memberValue(const std::string& text, std::size_t start, std::string_view key)
{
    if (start >= text.size() || text[start] != '{') return std::string::npos;
    int depth = 1;
    for (std::size_t i = start + 1; i < text.size() && depth > 0; ++i) {
        if (text[i] == '"') {
            const auto begin = ++i;
            bool escaped = false;
            for (; i < text.size(); ++i) {
                if (escaped) { escaped = false; continue; }
                if (text[i] == '\\') { escaped = true; continue; }
                if (text[i] == '"') break;
            }
            if (i == text.size()) return std::string::npos;
            const auto colon = skipWhitespace(text, i + 1);
            if (depth == 1 && std::string_view(text).substr(begin, i - begin) == key &&
                colon < text.size() && text[colon] == ':')
                return skipWhitespace(text, colon + 1);
        } else if (text[i] == '{' || text[i] == '[') ++depth;
        else if (text[i] == '}' || text[i] == ']') --depth;
    }
    return std::string::npos;
}

std::int64_t readDiskFreeMb(const std::string& text)
{
    const auto disk = memberValue(text, skipWhitespace(text, 0), "diskusage");
    const auto value = memberValue(text, disk, "free_mb");
    if (value == std::string::npos || value >= text.size() ||
        text[value] < '0' || text[value] > '9') return -1;
    std::int64_t result = -1;
    const char* end = text.data() + text.size();
    const auto parsed = std::from_chars(text.data() + value, end, result);
    if (parsed.ec != std::errc{} || result < 0) return -1;
    const auto next = skipWhitespace(text, static_cast<std::size_t>(parsed.ptr - text.data()));
    if (next >= text.size() || (text[next] != ',' && text[next] != '}')) return -1;
    return result;
}
}

VdrStatus RestfulApiStatusMapper::parseStatus(const std::string& json, const VdrConfig& config, int statusCode)
{
    VdrStatus status;
    status.enabled = config.enabled;
    status.mode = config.mode;
    status.host = config.host;
    status.port = config.port;
    status.state = statusCode == 200 && looksLikeJsonObject(json) ? "restfulapi" : "error";

    if (status.state == "restfulapi") status.diskFreeMb = readDiskFreeMb(json);
    return status;
}
