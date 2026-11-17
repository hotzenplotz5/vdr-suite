#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
// Standard epgsearch LSTS/EDIS format. Preserve every native field,
// including newer trailing fields invisible to the RESTfulAPI projection.
class EpgsearchStockRule
{
public:
    static bool nativeIdValid(const std::string& id)
    {
        return !id.empty() && id.size() <= 10U &&
            (id.size() == 1U || id.front() != '0') &&
            (id.size() < 10U || id <= "2147483647") &&
            std::all_of(id.begin(), id.end(), [](char c) {
                return c >= '0' && c <= '9';
            });
    }

    static bool singleLine(const std::string& text)
    {
        return !text.empty() && std::none_of(text.begin(), text.end(),
            [](char c) { return c == '\r' || c == '\n' || c == '\0'; });
    }

    static std::optional<EpgsearchStockRule> parse(
        const std::string& nativeId, const std::string& raw)
    {
        if (!nativeIdValid(nativeId) || !singleLine(raw)) return std::nullopt;
        EpgsearchStockRule out;
        std::size_t begin = 0U;
        for (;;)
        {
            const auto end = raw.find(':', begin);
            out.fields_.push_back(raw.substr(begin,
                end == std::string::npos ? end : end - begin));
            if (end == std::string::npos) break;
            begin = end + 1U;
        }
        // The stock 54-field form and its subsequent extensions share
        // these positions; don't truncate or manufacture default fields.
        if (out.fields_.size() < 54U || out.fields_[0] != nativeId ||
            out.fields_[1].empty() ||
            (out.fields_[15] != "0" && out.fields_[15] != "1" &&
             out.fields_[15] != "2")) return std::nullopt;
        return out;
    }

    std::string text() const
    {
        std::string out;
        for (std::size_t i = 0; i < fields_.size(); ++i)
        {
            if (i) out += ':';
            out += fields_[i];
        }
        return out;
    }

    std::string query() const
    {
        std::string decoded;
        const auto& encoded = fields_[1];
        const std::string marker = "!^pipe^!";
        for (std::size_t i = 0; i < encoded.size();)
        {
            if (encoded.compare(i, marker.size(), marker) == 0)
            {
                decoded += '|';
                i += marker.size();
            }
            else
            {
                decoded += encoded[i] == '|' ? ':' : encoded[i];
                ++i;
            }
        }
        return decoded;
    }

    bool active() const { return fields_[15] != "0"; }

    std::optional<EpgsearchStockRule> withQuery(
        const std::string& queryText) const
    {
        if (!singleLine(queryText)) return std::nullopt;
        EpgsearchStockRule out = *this;
        std::string encoded;
        for (char c : queryText)
        {
            if (c == '|') encoded += "!^pipe^!";
            else encoded += c == ':' ? '|' : c;
        }
        out.fields_[1] = std::move(encoded);
        if (out.query() != queryText) return std::nullopt;
        return out;
    }

    EpgsearchStockRule withActive(bool enabled) const
    {
        EpgsearchStockRule out = *this;
        out.fields_[15] = enabled ? "1" : "0";
        return out;
    }

private:
    std::vector<std::string> fields_;
};
} // namespace vdrsuite::agent
