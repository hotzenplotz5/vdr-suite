#include "ImageGalleryApi.h"
#include "LocalImageGalleryProvider.h"

#include <algorithm>
#include <map>
#include <regex>
#include <sstream>
#include <utility>

namespace vdrsuite::imagegallery {
namespace {
constexpr std::size_t OriginalBudget = 64U * 1024U * 1024U;

ApiResponse response(int status, std::string body,
                     std::string type = "application/json")
{
    return {status, std::move(type), std::move(body),
            {{"Cache-Control", "no-store"}, {"X-Content-Type-Options", "nosniff"}}};
}
ApiResponse problem(int status, const char* code)
{
    return response(status, std::string("{\"error\":{\"code\":\"") + code + "\"}}");
}
std::string quote(const std::string& value)
{
    std::string out = "\"";
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 32) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += static_cast<char>(c);
    }
    return out + '"';
}
int hexValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool decode(const std::string& encoded, std::string& out)
{
    out.clear();
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        char c = encoded[i];
        if (c == '%') {
            if (i + 2 >= encoded.size()) return false;
            const int a = hexValue(encoded[i+1]), b = hexValue(encoded[i+2]);
            if (a < 0 || b < 0) return false;
            c = static_cast<char>(a * 16 + b);
            i += 2;
        }
        if (c == '\0') return false;
        out += c;
    }
    return true;
}
bool query(const std::string& target, std::map<std::string,std::string>& result)
{
    if (target.size() > 16384) return false;
    const auto start = target.find('?');
    if (start == std::string::npos) return false;
    std::istringstream stream(target.substr(start+1));
    std::string pair;
    while (std::getline(stream,pair,'&')) {
        const auto equal = pair.find('=');
        if (equal == std::string::npos) return false;
        std::string value;
        if (!decode(pair.substr(equal+1),value) ||
            !result.emplace(pair.substr(0,equal),std::move(value)).second) return false;
    }
    return true;
}
std::string sourceJson(const std::vector<Source>& sources, bool admin)
{
    std::string out = "[";
    for (const auto& source : sources) {
        if (out.size() > 1) out += ',';
        out += "{\"id\":" + quote(source.id) + ",\"title\":" + quote(source.title);
        if (admin) out += ",\"root\":" + quote(source.root);
        out += '}';
    }
    return out + ']';
}
std::string entriesJson(const std::vector<Entry>& entries)
{
    std::string out = "[";
    for (const auto& entry : entries) {
        if (out.size() > 1) out += ',';
        out += "{\"name\":" + quote(entry.name) + ",\"relativePath\":" +
            quote(entry.relativePath) + ",\"directory\":" + (entry.directory ? "true" : "false") +
            ",\"cover\":" + quote(entry.cover) + '}';
    }
    return out + ']';
}
bool parseSources(const std::string& body, std::string& backend,
                  std::vector<Source>& sources)
{
    // Same bounded percent-encoded row transport used by the existing addon
    // source editor. Only this complete snapshot is persisted.
    if (body.size() > 32000) return false;
    static const std::regex format(
        R"IMG(^\{"backendId":"([A-Za-z0-9_.:-]{1,128})","entries":"([A-Za-z0-9%_.!~*'()-]*)"\}$)IMG");
    std::smatch match;
    if (!std::regex_match(body,match,format)) return false;
    backend = match[1].str();
    std::string rows;
    if (!decode(match[2].str(),rows)) return false;
    std::istringstream input(rows);
    std::string row;
    while (std::getline(input,row)) {
        const auto a = row.find('\t');
        if (a == std::string::npos) return false;
        const auto b = row.find('\t',a+1);
        if (b == std::string::npos || row.find('\t',b+1) != std::string::npos) return false;
        sources.push_back({row.substr(0,a),row.substr(a+1,b-a-1),row.substr(b+1)});
    }
    return true;
}
} // namespace

ImageGalleryApi::ImageGalleryApi(std::string localBackend, ImageGallerySourceSettings settings, Preview preview)
    : localBackend_(std::move(localBackend)), settings_(std::move(settings)), preview_(std::move(preview)) {}

ApiResponse ImageGalleryApi::get(const std::string& target, bool enabled) const
{
    std::map<std::string,std::string> args;
    if (!query(target,args) || !args.count("backendId")) return problem(400,"invalid_query");
    if (args.at("backendId") != localBackend_) return problem(404,"local_gallery_unavailable");
    const auto path = target.substr(0,target.find('?'));
    const bool admin = path == "/api/v1/addons/image/sources";
    const bool library = path == "/api/v1/addons/image/library";
    const bool sample = library && args.size() == 2 && args.count("sample") && args.at("sample") == "1";
    if (library && !enabled && (args.size() == 1 || sample))
        return response(200,sample ? "{\"enabled\":false,\"sources\":[],\"sample\":[]}" :
            "{\"enabled\":false,\"sources\":[]}");
    if (!admin && !enabled) return problem(404,"gallery_disabled");
    const auto snapshot = settings_.load();
    if (snapshot.state == SourceSettingsState::unavailable) return problem(503,"sources_unavailable");
    if (admin || library) {
        if (args.size() != 1 && !sample) return problem(400,"invalid_query");
        std::string body = std::string("{\"backendId\":") + quote(localBackend_) +
            ",\"enabled\":" + (enabled ? "true" : "false") +
            ",\"sources\":" + sourceJson(snapshot.sources,admin);
        if (sample) {
            body += ",\"sample\":[";
            bool first = true;
            for (const auto& photo : sampleLocalPhotos(snapshot.sources)) {
                if (!first) body += ',';
                first = false;
                body += "{\"sourceId\":" + quote(photo.sourceId) +
                    ",\"name\":" + quote(photo.name) +
                    ",\"relativePath\":" + quote(photo.relativePath) + '}';
            }
            body += ']';
        }
        return response(200,body + '}');
    }
    if (args.size() != 3 || !args.count("sourceId") || !args.count("path"))
        return problem(400,"invalid_query");
    const auto source = std::find_if(snapshot.sources.begin(), snapshot.sources.end(),
        [&](const Source& value) { return value.id == args.at("sourceId"); });
    if (source == snapshot.sources.end()) return problem(404,"source_not_found");
    if (path == "/api/v1/addons/image/browse") {
        const auto result = browseLocalDirectory(*source,args.at("path"));
        if (!result.success) return problem(404,"directory_unavailable");
        return response(200,"{\"entries\":" + entriesJson(result.entries) + '}');
    }
    if (path == "/api/v1/addons/image/photo" || path == "/api/v1/addons/image/preview") {
        auto photo = readLocalPhoto(*source,args.at("path"),OriginalBudget);
        if (photo.status == PhotoReadStatus::exceedsBudget) return problem(413,"photo_too_large");
        if (photo.status == PhotoReadStatus::unsupported) return problem(415,"unsupported_photo");
        if (photo.status != PhotoReadStatus::ready) return problem(404,"photo_unavailable");
        if (path == "/api/v1/addons/image/preview") {
            auto jpeg = preview_ ? preview_(photo.contentType,photo.bytes) : std::string{};
            if (jpeg.empty()) return problem(503,"preview_unavailable");
            return response(200,std::move(jpeg),"image/jpeg");
        }
        return response(200,std::move(photo.bytes),std::move(photo.contentType));
    }
    return problem(404,"unknown_gallery_resource");
}

ApiResponse ImageGalleryApi::save(const std::string& body) const
{
    std::string backend;
    std::vector<Source> sources;
    if (!parseSources(body,backend,sources)) return problem(422,"invalid_sources");
    if (backend != localBackend_) return problem(404,"local_gallery_unavailable");
    if (!settings_.save(sources)) return problem(422,"sources_not_saved");
    return response(200,"{\"backendId\":" + quote(backend) +
        ",\"sources\":" + sourceJson(sources,true) + '}');
}

} // namespace vdrsuite::imagegallery
