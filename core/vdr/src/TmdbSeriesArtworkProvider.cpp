#include "TmdbSeriesArtworkProvider.h"

#include "TmdbSeriesArtworkJson.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <limits>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
constexpr const char* ApiBase = "https://api.themoviedb.org/3";
constexpr const char* ImageBase = "https://image.tmdb.org/t/p/original";
std::atomic<unsigned long long> TemporaryCounter{0};

struct SelectedIdentity
{
    SeriesArtworkProviderCacheKey cacheKey;
    int tmdbSeriesId = 0;
    std::string externalSource;
    bool valid() const { return cacheKey.valid(); }
    bool direct() const { return tmdbSeriesId > 0; }
};

bool asciiDigits(const std::string& value)
{
    return !value.empty() && value.size() <= 10U &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= '0' && character <= '9';
        });
}

int positiveInt(const std::string& value)
{
    if (!asciiDigits(value)) return 0;
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || end == nullptr || *end != '\0' ||
        parsed <= 0 || parsed > std::numeric_limits<int>::max()) return 0;
    return static_cast<int>(parsed);
}

bool validImdbId(const std::string& value)
{
    return value.size() >= 9U && value.size() <= 14U &&
        value.compare(0, 2, "tt") == 0 &&
        std::all_of(value.begin() + 2, value.end(), [](unsigned char character) {
            return character >= '0' && character <= '9';
        });
}

SelectedIdentity selectIdentity(const EpgScraperMetadata& metadata)
{
    for (const auto& identity : metadata.externalIds)
    {
        if (identity.scope != EpgScraperExternalIdScope::Series ||
            identity.provider != EpgScraperExternalIdProvider::Tmdb) continue;
        const int id = positiveInt(identity.value);
        if (id > 0) return {{"tmdb", "tmdb", std::to_string(id)}, id, {}};
    }
    for (const auto& identity : metadata.externalIds)
    {
        if (identity.scope == EpgScraperExternalIdScope::Series &&
            identity.provider == EpgScraperExternalIdProvider::Imdb &&
            validImdbId(identity.value))
            return {{"tmdb", "imdb", identity.value}, 0, "imdb_id"};
    }
    for (const auto& identity : metadata.externalIds)
    {
        if (identity.scope != EpgScraperExternalIdScope::Series ||
            identity.provider != EpgScraperExternalIdProvider::Tvdb) continue;
        const int id = positiveInt(identity.value);
        if (id > 0) return {{"tmdb", "tvdb", std::to_string(id)}, 0, "tvdb_id"};
    }
    return {};
}

bool controlOrWhitespace(const std::string& value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return character <= 0x20U || character == 0x7fU;
    });
}

bool validLanguage(const std::string& value)
{
    if (value.size() != 2U && value.size() != 5U) return false;
    if (value[0] < 'a' || value[0] > 'z' || value[1] < 'a' || value[1] > 'z')
        return false;
    return value.size() == 2U ||
        (value[2] == '-' && value[3] >= 'A' && value[3] <= 'Z' &&
         value[4] >= 'A' && value[4] <= 'Z');
}

bool validImageLanguages(const std::string& value)
{
    if (value.empty() || value.size() > 64U) return false;
    std::vector<std::string> tokens;
    std::size_t start = 0;
    while (start <= value.size())
    {
        const std::size_t end = value.find(',', start);
        const std::string token = value.substr(
            start, end == std::string::npos ? std::string::npos : end - start);
        if (token != "null" &&
            (token.size() != 2U || token[0] < 'a' || token[0] > 'z' ||
             token[1] < 'a' || token[1] > 'z')) return false;
        if (std::find(tokens.begin(), tokens.end(), token) != tokens.end() ||
            tokens.size() >= 4U) return false;
        tokens.push_back(token);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return std::find(tokens.begin(), tokens.end(), "null") != tokens.end();
}

bool retryable(const ExternalArtworkHttpResponse& response)
{
    return response.transportError || response.statusCode == 429L ||
        response.statusCode == 500L || response.statusCode == 502L ||
        response.statusCode == 503L || response.statusCode == 504L;
}

ExternalArtworkHttpResponse requestWithRetry(
    IExternalArtworkHttpTransport& transport,
    const ExternalArtworkHttpRequest& request,
    const TmdbSeriesArtworkProviderConfig& config,
    const TmdbSeriesArtworkProvider::Sleeper& sleeper)
{
    ExternalArtworkHttpResponse response;
    for (int attempt = 0; attempt <= config.maximumRetries; ++attempt)
    {
        response = transport.perform(request);
        if (!retryable(response) || attempt == config.maximumRetries) return response;
        long long delay = static_cast<long long>(config.retryBackoffMs) << attempt;
        if (response.retryAfterSeconds > 0)
            delay = std::max(delay, response.retryAfterSeconds * 1000LL);
        sleeper(std::chrono::milliseconds(std::min(delay, 2000LL)));
    }
    return response;
}

bool jsonReady(const ExternalArtworkHttpResponse& response)
{
    return !response.transportError && response.statusCode == 200L &&
        response.contentType == "application/json" && !response.body.empty();
}

std::string encodeQuery(const std::string& value)
{
    static const char Hex[] = "0123456789ABCDEF";
    std::string encoded;
    for (const unsigned char character : value)
    {
        if ((character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' || character == '.' || character == '~')
            encoded.push_back(static_cast<char>(character));
        else
        {
            encoded.push_back('%');
            encoded.push_back(Hex[character >> 4U]);
            encoded.push_back(Hex[character & 0x0fU]);
        }
    }
    return encoded;
}

long long expiry(long long now, int ttl)
{
    if (now < 0 || ttl <= 0 || now > std::numeric_limits<long long>::max() - ttl)
        return std::numeric_limits<long long>::max();
    return now + ttl;
}

void cacheFailure(
    ISeriesArtworkProviderCache& cache,
    const SeriesArtworkProviderCacheKey& key,
    long long now,
    const TmdbSeriesArtworkProviderConfig& config,
    bool notFound)
{
    cache.store(
        key,
        notFound ? SeriesArtworkProviderCacheOutcome::NotFound
                 : SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable,
        expiry(now, notFound ? config.negativeCacheTtlSeconds
                             : config.transientCacheTtlSeconds));
}

std::uint64_t fnv1a(const std::string& value)
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char byte : value)
    {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string hex64(std::uint64_t value)
{
    static const char Hex[] = "0123456789abcdef";
    std::string output(16, '0');
    for (int index = 15; index >= 0; --index)
    {
        output[static_cast<std::size_t>(index)] = Hex[value & 0x0fU];
        value >>= 4U;
    }
    return output;
}

int openDirectoryNoFollow(const std::filesystem::path& path)
{
    if (!path.is_absolute() || path == path.root_path()) return -1;
    int current = ::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (current < 0) return -1;
    for (const auto& component : path.lexically_normal().relative_path())
    {
        const std::string name = component.string();
        if (name.empty() || name == "." || name == "..")
        {
            ::close(current);
            return -1;
        }
        const int next = ::openat(
            current, name.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        ::close(current);
        if (next < 0) return -1;
        current = next;
    }
    return current;
}

bool writeAll(int descriptor, const std::string& body)
{
    std::size_t offset = 0;
    while (offset < body.size())
    {
        const ssize_t written = ::write(
            descriptor, body.data() + offset, body.size() - offset);
        if (written < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (written == 0) return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

std::string storeIncoming(
    const std::string& incomingRoot,
    int seriesId,
    const std::string& remotePath,
    const std::string& body)
{
    const std::filesystem::path root =
        std::filesystem::path(incomingRoot).lexically_normal();
    const int directory = openDirectoryNoFollow(root);
    if (directory < 0) return {};
    const std::string finalName = "tmdb-" + std::to_string(seriesId) + "-" +
        hex64(fnv1a(remotePath)) + ".candidate";
    const std::string temporaryName = "." + finalName + "." +
        std::to_string(::getpid()) + "." +
        std::to_string(TemporaryCounter.fetch_add(1)) + ".tmp";
    const int output = ::openat(
        directory, temporaryName.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
        S_IRUSR | S_IWUSR);
    if (output < 0)
    {
        ::close(directory);
        return {};
    }
    bool ok = writeAll(output, body) && ::fsync(output) == 0;
    if (::close(output) != 0) ok = false;
    if (ok) ok = ::renameat(
        directory, temporaryName.c_str(), directory, finalName.c_str()) == 0;
    if (ok) ok = ::fsync(directory) == 0;
    if (!ok) ::unlinkat(directory, temporaryName.c_str(), 0);
    ::close(directory);
    return ok ? (root / finalName).string() : std::string{};
}

long long systemClock()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

TmdbSeriesArtworkProvider::TmdbSeriesArtworkProvider(
    IExternalArtworkHttpTransport& transport,
    ISeriesArtworkProviderCache& cache,
    TmdbSeriesArtworkProviderConfig config,
    Clock clock,
    Sleeper sleeper)
    : transport_(transport), cache_(cache), config_(std::move(config)),
      clock_(clock ? std::move(clock) : Clock(systemClock)),
      sleeper_(sleeper ? std::move(sleeper) : Sleeper(
          [](std::chrono::milliseconds delay) { std::this_thread::sleep_for(delay); }))
{
}

bool TmdbSeriesArtworkProvider::configurationValid(
    const TmdbSeriesArtworkProviderConfig& config)
{
    const auto root = std::filesystem::path(config.incomingRoot).lexically_normal();
    return !config.readAccessToken.empty() && config.readAccessToken.size() <= 4096U &&
        !controlOrWhitespace(config.readAccessToken) && validLanguage(config.language) &&
        validImageLanguages(config.includeImageLanguages) &&
        !config.incomingRoot.empty() && config.incomingRoot.size() <= 4096U &&
        root.is_absolute() && root != root.root_path() &&
        config.connectTimeoutMs >= 100 && config.connectTimeoutMs <= 10000 &&
        config.totalTimeoutMs >= config.connectTimeoutMs && config.totalTimeoutMs <= 30000 &&
        config.maximumRetries >= 0 && config.maximumRetries <= 2 &&
        config.retryBackoffMs >= 50 && config.retryBackoffMs <= 2000 &&
        config.negativeCacheTtlSeconds >= 60 &&
        config.negativeCacheTtlSeconds <= 7 * 24 * 60 * 60 &&
        config.transientCacheTtlSeconds >= 10 && config.transientCacheTtlSeconds <= 3600 &&
        config.maximumJsonBytes >= 1024U && config.maximumJsonBytes <= 2U * 1024U * 1024U &&
        config.maximumImageBytes >= 1024U && config.maximumImageBytes <= 32U * 1024U * 1024U;
}

SeriesArtworkFallbackResolution TmdbSeriesArtworkProvider::resolve(
    const std::string&,
    const VdrEvent&,
    const EpgScraperMetadata& metadata)
{
    SeriesArtworkFallbackResolution result;
    if (!configurationValid(config_) || !metadata.valid() ||
        metadata.mediaType != EpgScraperMediaType::Series ||
        metadata.preferredArtwork.valid()) return result;

    const SelectedIdentity identity = selectIdentity(metadata);
    if (!identity.valid()) return result;
    result.attempted = true;
    const long long now = clock_();
    if (cache_.find(identity.cacheKey, now).active(now)) return result;

    int seriesId = identity.tmdbSeriesId;
    if (!identity.direct())
    {
        ExternalArtworkHttpRequest request;
        request.url = std::string(ApiBase) + "/find/" +
            encodeQuery(identity.cacheKey.identityValue) + "?external_source=" +
            identity.externalSource + "&language=" + encodeQuery(config_.language);
        request.bearerToken = config_.readAccessToken;
        request.accept = "application/json";
        request.connectTimeoutMs = config_.connectTimeoutMs;
        request.totalTimeoutMs = config_.totalTimeoutMs;
        request.maximumResponseBytes = config_.maximumJsonBytes;
        const auto response = requestWithRetry(transport_, request, config_, sleeper_);
        if (!jsonReady(response))
        {
            cacheFailure(cache_, identity.cacheKey, now, config_, response.statusCode == 404L);
            return result;
        }
        if (!parseTmdbFindSeriesId(response.body, config_.maximumJsonBytes, seriesId))
        {
            cacheFailure(cache_, identity.cacheKey, now, config_, false);
            return result;
        }
        if (seriesId <= 0)
        {
            cacheFailure(cache_, identity.cacheKey, now, config_, true);
            return result;
        }
    }

    ExternalArtworkHttpRequest images;
    images.url = std::string(ApiBase) + "/tv/" + std::to_string(seriesId) +
        "/images?language=" + encodeQuery(config_.language) +
        "&include_image_language=" + encodeQuery(config_.includeImageLanguages);
    images.bearerToken = config_.readAccessToken;
    images.accept = "application/json";
    images.connectTimeoutMs = config_.connectTimeoutMs;
    images.totalTimeoutMs = config_.totalTimeoutMs;
    images.maximumResponseBytes = config_.maximumJsonBytes;
    const auto imagesResponse = requestWithRetry(transport_, images, config_, sleeper_);
    if (!jsonReady(imagesResponse))
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, imagesResponse.statusCode == 404L);
        return result;
    }

    TmdbSeriesBackdrop selected;
    if (!parseTmdbSeriesBackdrop(
            imagesResponse.body, config_.maximumJsonBytes, config_.language, selected))
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, false);
        return result;
    }
    if (selected.filePath.empty())
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, true);
        return result;
    }

    ExternalArtworkHttpRequest image;
    image.url = std::string(ImageBase) + selected.filePath;
    image.accept = "image/jpeg,image/png";
    image.connectTimeoutMs = config_.connectTimeoutMs;
    image.totalTimeoutMs = config_.totalTimeoutMs;
    image.maximumResponseBytes = config_.maximumImageBytes;
    const auto imageResponse = requestWithRetry(transport_, image, config_, sleeper_);
    if (imageResponse.transportError || imageResponse.statusCode != 200L ||
        imageResponse.body.empty())
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, false);
        return result;
    }

    const std::string path = storeIncoming(
        config_.incomingRoot, seriesId, selected.filePath, imageResponse.body);
    if (path.empty())
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, false);
        return result;
    }

    cache_.remove(identity.cacheKey);
    result.found = true;
    result.artwork.available = true;
    result.artwork.provider = "tmdb";
    result.artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    result.artwork.path = path;
    result.artwork.width = selected.width;
    result.artwork.height = selected.height;
    return result;
}
