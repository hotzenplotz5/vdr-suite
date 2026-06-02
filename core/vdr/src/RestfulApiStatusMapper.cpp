#include "RestfulApiStatusMapper.h"

#include <cctype>
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

}

VdrStatus RestfulApiStatusMapper::parseStatus(const std::string& json, const VdrConfig& config, int statusCode)
{
    VdrStatus status;
    status.enabled = config.enabled;
    status.mode = config.mode;
    status.host = config.host;
    status.port = config.port;
    status.state = statusCode == 200 && looksLikeJsonObject(json) ? "restfulapi" : "error";

    return status;
}
