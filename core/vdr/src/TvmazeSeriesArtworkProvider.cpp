#include "TvmazeSeriesArtworkProvider.h"

#include "TvmazeSeriesArtworkJson.h"

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
constexpr const char* ApiBase = "https://api.tvmaze.com";
std::atomic<unsigned long long> TemporaryCounter{0};

struct SelectedIdentity
{
    SeriesArtworkProviderCacheKey cacheKey;
    std::string lookupParameter;

    bool valid() const
    {
        return cacheKey.valid() && !lookupParameter.empty();
    }
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
    if (!asciiDigits(value) || (value.size() > 1U && value.front() == '0'))
        return 0;
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || end == nullptr || *end != '\0' ||
        parsed <= 0 || parsed > std::numeric_limits<int>::max())
    {
        return 0;
    }
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
    // TVScraper's public service contract qualifies a negative database ID
    // as TheTVDB. Prefer that exact series identity because TVmaze coverage
    // for regional shows is often better by TheTVDB ID than by IMDb ID.
    if (metadata.provider == "tvscraper" &&
        metadata.mediaType == EpgScraperMediaType::Series &&
        metadata.providerId < 0)
    {
        const long long tvdbId =
            -static_cast<long long>(metadata.providerId);
        if (tvdbId > 0 &&
            tvdbId <= std::numeric_limits<int>::max())
        {
            return {
                {"tvmaze", "tvdb", std::to_string(tvdbId)},
                "thetvdb"
            };
        }
    }

    for (const auto& identity : metadata.externalIds)
    {
        if (identity.scope == EpgScraperExternalIdScope::Series &&
            identity.provider == EpgScraperExternalIdProvider::Imdb &&
            validImdbId(identity.value))
        {
            return {{"tvmaze", "imdb", identity.value}, "imdb"};
        }
    }

    for (const auto& identity : metadata.externalIds)
    {
        if (identity.scope != EpgScraperExternalIdScope::Series ||
            identity.provider != EpgScraperExternalIdProvider::Tvdb)
        {
            continue;
        }
        const int id = positiveInt(identity.value);
        if (id > 0)
        {
            return {
                {"tvmaze", "tvdb", std::to_string(id)},
                "thetvdb"
            };
        }
    }
    return {};
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
    const TvmazeSeriesArtworkProviderConfig& config,
    const TvmazeSeriesArtworkProvider::Sleeper& sleeper)
{
    ExternalArtworkHttpResponse response;
    for (int attempt = 0; attempt <= config.maximumRetries; ++attempt)
    {
        response = transport.perform(request);
        if (!retryable(response) || attempt == config.maximumRetries)
            return response;

        long long delay =
            static_cast<long long>(config.retryBackoffMs) << attempt;
        if (response.retryAfterSeconds > 0)
            delay = std::max(delay, response.retryAfterSeconds * 1000LL);
        sleeper(std::chrono::milliseconds(std::min(delay, 5000LL)));
    }
    return response;
}

bool jsonReady(const ExternalArtworkHttpResponse& response)
{
    return !response.transportError && response.statusCode == 200L &&
        response.contentType == "application/json" && !response.body.empty();
}

long long expiry(long long now, int ttl)
{
    if (now < 0 || ttl <= 0 ||
        now > std::numeric_limits<long long>::max() - ttl)
    {
        return std::numeric_limits<long long>::max();
    }
    return now + ttl;
}

void cacheFailure(
    ISeriesArtworkProviderCache& cache,
    const SeriesArtworkProviderCacheKey& key,
    long long now,
    const TvmazeSeriesArtworkProviderConfig& config,
    bool notFound)
{
    cache.store(
        key,
        notFound ? SeriesArtworkProviderCacheOutcome::NotFound
                 : SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable,
        expiry(
            now,
            notFound ? config.negativeCacheTtlSeconds
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
            current,
            name.c_str(),
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
            descriptor,
            body.data() + offset,
            body.size() - offset);
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
    const std::string& remoteUrl,
    const std::string& body)
{
    const std::filesystem::path root =
        std::filesystem::path(incomingRoot).lexically_normal();
    const int directory = openDirectoryNoFollow(root);
    if (directory < 0) return {};

    const std::string finalName =
        "tvmaze-" + std::to_string(seriesId) + "-" +
        hex64(fnv1a(remoteUrl)) + ".candidate";
    const std::string temporaryName =
        "." + finalName + "." + std::to_string(::getpid()) + "." +
        std::to_string(TemporaryCounter.fetch_add(1)) + ".tmp";

    const int output = ::openat(
        directory,
        temporaryName.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
        S_IRUSR | S_IWUSR);
    if (output < 0)
    {
        ::close(directory);
        return {};
    }

    bool ok = writeAll(output, body) && ::fsync(output) == 0;
    if (::close(output) != 0) ok = false;
    if (ok)
    {
        ok = ::renameat(
            directory,
            temporaryName.c_str(),
            directory,
            finalName.c_str()) == 0;
    }
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

TvmazeSeriesArtworkProvider::TvmazeSeriesArtworkProvider(
    IExternalArtworkHttpTransport& transport,
    ISeriesArtworkProviderCache& cache,
    TvmazeSeriesArtworkProviderConfig config,
    Clock clock,
    Sleeper sleeper)
    : transport_(transport),
      cache_(cache),
      config_(std::move(config)),
      clock_(clock ? std::move(clock) : Clock(systemClock)),
      sleeper_(
          sleeper
              ? std::move(sleeper)
              : Sleeper([](std::chrono::milliseconds delay) {
                    std::this_thread::sleep_for(delay);
                }))
{
}

bool TvmazeSeriesArtworkProvider::configurationValid(
    const TvmazeSeriesArtworkProviderConfig& config)
{
    const auto root =
        std::filesystem::path(config.incomingRoot).lexically_normal();
    return !config.incomingRoot.empty() &&
        config.incomingRoot.size() <= 4096U &&
        root.is_absolute() && root != root.root_path() &&
        config.connectTimeoutMs >= 100 &&
        config.connectTimeoutMs <= 10000 &&
        config.totalTimeoutMs >= config.connectTimeoutMs &&
        config.totalTimeoutMs <= 30000 &&
        config.maximumRetries >= 0 &&
        config.maximumRetries <= 2 &&
        config.retryBackoffMs >= 50 &&
        config.retryBackoffMs <= 5000 &&
        config.negativeCacheTtlSeconds >= 60 &&
        config.negativeCacheTtlSeconds <= 7 * 24 * 60 * 60 &&
        config.transientCacheTtlSeconds >= 10 &&
        config.transientCacheTtlSeconds <= 3600 &&
        config.maximumJsonBytes >= 1024U &&
        config.maximumJsonBytes <= 4U * 1024U * 1024U &&
        config.maximumImageBytes >= 1024U &&
        config.maximumImageBytes <= 32U * 1024U * 1024U;
}

SeriesArtworkFallbackResolution TvmazeSeriesArtworkProvider::resolve(
    const std::string&,
    const VdrEvent&,
    const EpgScraperMetadata& metadata)
{
    SeriesArtworkFallbackResolution result;
    if (!configurationValid(config_) || !metadata.valid() ||
        metadata.mediaType != EpgScraperMediaType::Series ||
        metadata.preferredArtwork.valid())
    {
        return result;
    }

    const SelectedIdentity identity = selectIdentity(metadata);
    if (!identity.valid()) return result;

    result.attempted = true;
    const long long now = clock_();
    if (cache_.find(identity.cacheKey, now).active(now)) return result;

    ExternalArtworkHttpRequest lookup;
    lookup.url =
        std::string(ApiBase) + "/lookup/shows?" +
        identity.lookupParameter + "=" + identity.cacheKey.identityValue;
    lookup.accept = "application/json";
    lookup.connectTimeoutMs = config_.connectTimeoutMs;
    lookup.totalTimeoutMs = config_.totalTimeoutMs;
    lookup.maximumResponseBytes = config_.maximumJsonBytes;

    const auto lookupResponse =
        requestWithRetry(transport_, lookup, config_, sleeper_);
    if (lookupResponse.transportError ||
        (lookupResponse.statusCode != 301L &&
         lookupResponse.statusCode != 302L))
    {
        cacheFailure(
            cache_,
            identity.cacheKey,
            now,
            config_,
            !lookupResponse.transportError &&
                lookupResponse.statusCode == 404L);
        return result;
    }

    int seriesId = 0;
    if (!parseTvmazeShowLocation(lookupResponse.location, seriesId))
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, false);
        return result;
    }

    ExternalArtworkHttpRequest images;
    images.url =
        std::string(ApiBase) + "/shows/" +
        std::to_string(seriesId) + "/images";
    images.accept = "application/json";
    images.connectTimeoutMs = config_.connectTimeoutMs;
    images.totalTimeoutMs = config_.totalTimeoutMs;
    images.maximumResponseBytes = config_.maximumJsonBytes;

    const auto imagesResponse =
        requestWithRetry(transport_, images, config_, sleeper_);
    if (!jsonReady(imagesResponse))
    {
        cacheFailure(
            cache_,
            identity.cacheKey,
            now,
            config_,
            !imagesResponse.transportError &&
                imagesResponse.statusCode == 404L);
        return result;
    }

    TvmazeSeriesImage selected;
    if (!parseTvmazeSeriesImage(
            imagesResponse.body,
            config_.maximumJsonBytes,
            selected))
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, false);
        return result;
    }
    if (selected.url.empty())
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, true);
        return result;
    }

    ExternalArtworkHttpRequest image;
    image.url = selected.url;
    image.accept = "image/jpeg,image/png";
    image.connectTimeoutMs = config_.connectTimeoutMs;
    image.totalTimeoutMs = config_.totalTimeoutMs;
    image.maximumResponseBytes = config_.maximumImageBytes;

    const auto imageResponse =
        requestWithRetry(transport_, image, config_, sleeper_);
    if (imageResponse.transportError ||
        imageResponse.statusCode != 200L ||
        imageResponse.body.empty() ||
        (imageResponse.contentType != "image/jpeg" &&
         imageResponse.contentType != "image/png"))
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, false);
        return result;
    }

    const std::string path = storeIncoming(
        config_.incomingRoot,
        seriesId,
        selected.url,
        imageResponse.body);
    if (path.empty())
    {
        cacheFailure(cache_, identity.cacheKey, now, config_, false);
        return result;
    }

    cache_.remove(identity.cacheKey);
    result.found = true;
    result.artwork.available = true;
    result.artwork.provider = "tvmaze";
    result.artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    result.artwork.path = path;
    result.artwork.width = selected.width;
    result.artwork.height = selected.height;
    return result;
}
