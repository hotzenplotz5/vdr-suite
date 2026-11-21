#include "SuiteBridgeRecordingMetadataResolver.h"
#include "VdrRecordingNativeIdentity.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace {
class Error final : public std::runtime_error { public: using runtime_error::runtime_error; };

class Parser final {
public:
    explicit Parser(const std::string& s) : s_(s) {}
    bool done() const noexcept { return p_ == s_.size(); }
    bool take(char c) noexcept { if (p_ < s_.size() && s_[p_] == c) { ++p_; return true; } return false; }
    void need(char c) { if (!take(c)) fail(std::string("expected '") + c + "'"); }
    void literal(const char* value) {
        const std::string v(value);
        if (s_.compare(p_, v.size(), v) != 0) fail("expected " + v);
        p_ += v.size();
    }
    void field(const char* name, bool first = false) {
        if (!first) need(',');
        need('"'); literal(name); need('"'); need(':');
    }
    bool boolean() {
        if (s_.compare(p_, 4, "true") == 0) { p_ += 4; return true; }
        if (s_.compare(p_, 5, "false") == 0) { p_ += 5; return false; }
        fail("expected boolean"); return false;
    }
    int integer() {
        const std::string token = numberToken(false);
        errno = 0; char* end = nullptr;
        const long value = std::strtol(token.c_str(), &end, 10);
        if (errno || !end || *end || value < std::numeric_limits<int>::min() ||
            value > std::numeric_limits<int>::max()) fail("integer out of range");
        return static_cast<int>(value);
    }
    double number() {
        const std::string token = numberToken(true);
        errno = 0; char* end = nullptr;
        const double value = std::strtod(token.c_str(), &end);
        if (errno || !end || *end || !std::isfinite(value)) fail("number out of range");
        return value;
    }
    std::string string() {
        need('"'); std::string out;
        while (p_ < s_.size()) {
            const unsigned char c = static_cast<unsigned char>(s_[p_++]);
            if (c == '"') return out;
            if (c < 0x20) fail("control character in string");
            if (c != '\\') { out.push_back(static_cast<char>(c)); continue; }
            if (p_ >= s_.size()) fail("truncated escape");
            switch (s_[p_++]) {
            case '"': out.push_back('"'); break; case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break; case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break; case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break; case 't': out.push_back('\t'); break;
            case 'u': unicode(out); break; default: fail("invalid escape");
            }
        }
        fail("unterminated string"); return {};
    }
private:
    [[noreturn]] void fail(const std::string& message) const {
        throw Error(message + " at byte " + std::to_string(p_));
    }
    std::string numberToken(bool decimal) {
        const std::size_t start = p_;
        if (p_ < s_.size() && s_[p_] == '-') ++p_;
        if (p_ >= s_.size() || s_[p_] < '0' || s_[p_] > '9') fail("expected number");
        if (s_[p_] == '0') ++p_; else while (p_ < s_.size() && s_[p_] >= '0' && s_[p_] <= '9') ++p_;
        if (decimal && p_ < s_.size() && s_[p_] == '.') {
            ++p_; const std::size_t digits = p_;
            while (p_ < s_.size() && s_[p_] >= '0' && s_[p_] <= '9') ++p_;
            if (p_ == digits) fail("invalid fraction");
        }
        if (decimal && p_ < s_.size() && (s_[p_] == 'e' || s_[p_] == 'E')) {
            ++p_; if (p_ < s_.size() && (s_[p_] == '+' || s_[p_] == '-')) ++p_;
            const std::size_t digits = p_;
            while (p_ < s_.size() && s_[p_] >= '0' && s_[p_] <= '9') ++p_;
            if (p_ == digits) fail("invalid exponent");
        }
        return s_.substr(start, p_ - start);
    }
    unsigned hex4() {
        if (p_ + 4 > s_.size()) fail("truncated unicode escape");
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            const unsigned char c = static_cast<unsigned char>(s_[p_++]); value <<= 4;
            if (c >= '0' && c <= '9') value += c - '0';
            else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
            else fail("invalid unicode escape");
        }
        return value;
    }
    void unicode(std::string& out) {
        const unsigned v = hex4();
        if (v >= 0xD800 && v <= 0xDFFF) fail("surrogate escape is unsupported");
        if (v <= 0x7F) out.push_back(static_cast<char>(v));
        else if (v <= 0x7FF) { out.push_back(static_cast<char>(0xC0 | (v >> 6))); out.push_back(static_cast<char>(0x80 | (v & 0x3F))); }
        else { out.push_back(static_cast<char>(0xE0 | (v >> 12))); out.push_back(static_cast<char>(0x80 | ((v >> 6) & 0x3F))); out.push_back(static_cast<char>(0x80 | (v & 0x3F))); }
    }
    const std::string& s_; std::size_t p_ = 0;
};

void bounded(const std::string& value, std::size_t max, const char* field) {
    if (value.size() > max) throw Error(std::string(field) + " exceeds maximum length");
}

std::vector<std::string> strings(Parser& p, std::size_t max, const char* field) {
    std::vector<std::string> out; p.need('['); bool first = true;
    while (!p.take(']')) {
        if (!first) p.need(',');
        if (out.size() >= max) throw Error(std::string(field) + " exceeds maximum count");
        out.push_back(p.string()); bounded(out.back(), 256, field); first = false;
    }
    return out;
}

VdrRecordingNativeArtwork artwork(Parser& p, bool oriented) {
    VdrRecordingNativeArtwork a; p.need('{');
    if (oriented) { p.field("orientation", true); a.orientation = p.string(); p.field("artwork"); p.need('{'); }
    else p.field("available", true);
    if (oriented) { p.field("available", true); a.available = p.boolean(); } else a.available = p.boolean();
    p.field("provider"); a.provider = p.string(); p.field("path"); a.path = p.string();
    p.field("width"); a.width = p.integer(); p.field("height"); a.height = p.integer(); p.need('}');
    if (oriented) p.need('}');
    bounded(a.provider, 32, "artwork.provider"); bounded(a.path, 4096, "artwork.path");
    if (a.available) {
        if (a.provider != "tvscraper" || a.path.empty() || a.width <= 0 || a.height <= 0) throw Error("invalid available artwork");
    } else if (a.provider != "none" || !a.path.empty() || a.width || a.height) throw Error("invalid unavailable artwork");
    return a;
}

std::vector<VdrRecordingNativePerson> people(Parser& p) {
    std::vector<VdrRecordingNativePerson> out; p.need('['); bool first = true;
    while (!p.take(']')) {
        if (!first) p.need(',');
        if (out.size() >= VdrRecordingNativeMetadata::MaximumPeople) throw Error("people exceeds maximum count");
        VdrRecordingNativePerson v; p.need('{'); p.field("role", true); v.role = p.string();
        p.field("name"); v.name = p.string(); p.field("characterName"); v.characterName = p.string();
        p.field("image"); v.image = artwork(p, false); p.need('}');
        bounded(v.role, 32, "person.role"); bounded(v.name, 512, "person.name"); bounded(v.characterName, 512, "person.characterName");
        if (v.name.empty()) throw Error("person name is empty");
        out.push_back(std::move(v)); first = false;
    }
    return out;
}

std::vector<VdrRecordingNativeArtwork> images(Parser& p) {
    std::vector<VdrRecordingNativeArtwork> out; p.need('['); bool first = true;
    while (!p.take(']')) {
        if (!first) p.need(',');
        if (out.size() >= VdrRecordingNativeMetadata::MaximumImages) throw Error("images exceeds maximum count");
        auto v = artwork(p, true);
        if (!v.available || (v.orientation != "landscape" && v.orientation != "banner" && v.orientation != "portrait")) throw Error("invalid gallery image");
        out.push_back(std::move(v)); first = false;
    }
    return out;
}

VdrRecordingNativeMetadata payload(const std::string& key, const std::string& json) {
    if (json.empty() ||
        json.size() > VdrRecordingNativeMetadata::MaximumPayloadBytes)
    {
        throw Error("payload size is invalid");
    }
    Parser p(json); VdrRecordingNativeMetadata m; p.need('{');
#define I(name) p.field(#name); m.name = p.integer()
#define S(name) p.field(#name); m.name = p.string()
#define N(name) p.field(#name); m.name = p.number()
    p.field("schema", true); m.schema = p.integer(); p.field("found"); m.found = p.boolean();
    S(reason); S(provider); I(recordingIdentitySchema); S(recordingKey); S(mediaType); I(providerId);
    I(seasonNumber); I(episodeNumber); I(absoluteEpisodeNumber); I(runtimeMinutes); I(durationDeviationMinutes);
    I(scraperHd); I(scraperLanguage); N(popularity); N(voteAverage); I(voteCount);
    p.field("adult"); m.adult = p.boolean(); I(collectionId); I(lastSeason);
    S(title); S(originalTitle); S(episodeName); S(tagline); S(overview); S(releaseDate); S(firstAired); S(imdbId); S(status); S(collectionName);
#undef I
#undef S
#undef N
    p.field("genres"); m.genres = strings(p, VdrRecordingNativeMetadata::MaximumGenres, "genres");
    p.field("productionCountries"); m.productionCountries = strings(p, VdrRecordingNativeMetadata::MaximumCountries, "productionCountries");
    p.field("networks"); m.networks = strings(p, VdrRecordingNativeMetadata::MaximumNetworks, "networks");
    p.field("preferredArtwork"); m.preferredArtwork = artwork(p, false);
    p.field("people"); m.people = people(p); p.field("images"); m.images = images(p); p.need('}');
    if (!p.done()) throw Error("trailing payload data");
    bounded(m.reason,64,"reason"); bounded(m.provider,32,"provider"); bounded(m.recordingKey,32,"recordingKey"); bounded(m.mediaType,32,"mediaType");
    bounded(m.title,1024,"title"); bounded(m.originalTitle,1024,"originalTitle"); bounded(m.episodeName,1024,"episodeName");
    bounded(m.tagline,2048,"tagline"); bounded(m.overview,8192,"overview"); bounded(m.releaseDate,64,"releaseDate");
    bounded(m.firstAired,64,"firstAired"); bounded(m.imdbId,128,"imdbId"); bounded(m.status,128,"status"); bounded(m.collectionName,1024,"collectionName");
    if (m.schema != VdrRecordingNativeMetadata::SupportedSchema ||
        m.recordingIdentitySchema != VdrRecordingNativeMetadata::SupportedIdentitySchema ||
        m.recordingKey != key || !VdrRecordingNativeIdentity::isValidKey(m.recordingKey)) throw Error("schema or recording identity mismatch");
    if (m.found) {
        if (m.reason != "none" || m.provider != "tvscraper" || (m.mediaType != "movie" && m.mediaType != "series") || m.providerId == 0) throw Error("invalid found metadata state");
        m.availability = VdrRecordingNativeMetadataAvailability::Found;
    } else {
        if ((m.reason != "recording-not-found" && m.reason != "identity-ambiguous" && m.reason != "provider-no-match") ||
            m.provider != "none" || m.mediaType != "none" || m.providerId || !m.people.empty() || m.preferredArtwork.available || !m.images.empty()) throw Error("invalid not-found metadata state");
        m.availability = VdrRecordingNativeMetadataAvailability::NotFound;
    }
    return m;
}
}

SuiteBridgeRecordingMetadataResolver::SuiteBridgeRecordingMetadataResolver(ISuiteBridgeRecordingMetadataTransport& transport) : transport_(transport) {}

VdrRecordingNativeMetadata SuiteBridgeRecordingMetadataResolver::resolve(const std::string& key) {
    if (!VdrRecordingNativeIdentity::isValidKey(key)) {
        VdrRecordingNativeMetadata m; m.availability = VdrRecordingNativeMetadataAvailability::InvalidPayload; m.diagnostic = "invalid recording key"; return m;
    }
    return parseReply(key, transport_.requestRecordingMetadata(key));
}

VdrRecordingNativeMetadata SuiteBridgeRecordingMetadataResolver::parseReply(const std::string& key, const SuiteBridgeRecordingMetadataCommandReply& reply) {
    if (!reply.transportSucceeded) {
        VdrRecordingNativeMetadata m; m.recordingKey = key;
        m.availability = reply.replyCode == 451 ? VdrRecordingNativeMetadataAvailability::ProviderUnavailable : VdrRecordingNativeMetadataAvailability::TransportError;
        m.diagnostic = reply.payload.empty() ? "SuiteBridge recording metadata transport failed" : reply.payload; return m;
    }
    if (reply.replyCode != 250) {
        VdrRecordingNativeMetadata m; m.availability = VdrRecordingNativeMetadataAvailability::TransportError; m.recordingKey = key; m.diagnostic = "unexpected SuiteBridge reply code"; return m;
    }
    try { return payload(key, reply.payload); }
    catch (const std::exception& e) {
        VdrRecordingNativeMetadata m; m.availability = VdrRecordingNativeMetadataAvailability::InvalidPayload; m.recordingKey = key; m.diagnostic = e.what(); return m;
    }
}
