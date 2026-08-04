#include "TmdbRecordingMetadataCandidateProvider.h"

#include "IExternalArtworkHttpTransport.h"
#include "TmdbRecordingMetadataCandidateJson.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <string>
#include <thread>
#include <utility>

namespace
{
constexpr const char* ApiBase = "https://api.themoviedb.org/3";

bool controlOrWhitespace(const std::string& value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return character <= 0x20U || character == 0x7fU;
    });
}

bool validLanguage(const std::string& value)
{
    if (value.size() != 2U && value.size() != 5U) return false;
    if (value[0] < 'a' || value[0] > 'z' ||
        value[1] < 'a' || value[1] > 'z') return false;
    return value.size() == 2U ||
        (value[2] == '-' && value[3] >= 'A' && value[3] <= 'Z' &&
         value[4] >= 'A' && value[4] <= 'Z');
}

bool validQuery(const std::string& value)
{
    if (value.size() < 2U || value.size() > 256U) return false;
    return std::none_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U || character == 0x7fU;
    });
}

bool digits(const std::string& value)
{
    return !value.empty() && value.size() <= 16U &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= '0' && character <= '9';
        });
}

std::string encodeQuery(const std::string& value)
{
    static const char Hex[] = "0123456789ABCDEF";
    std::string encoded;
    encoded.reserve(value.size() * 3U);
    for (const unsigned char character : value)
    {
        if ((character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' || character == '.' ||
            character == '~')
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

bool retryable(const ExternalArtworkHttpResponse& response)
{
    return response.transportError || response.statusCode == 429L ||
        response.statusCode == 500L || response.statusCode == 502L ||
        response.statusCode == 503L || response.statusCode == 504L;
}

bool jsonReady(const ExternalArtworkHttpResponse& response)
{
    return !response.transportError && response.statusCode == 200L &&
        response.contentType.compare(0, 16, "application/json") == 0 &&
        !response.body.empty();
}

std::string responseError(const ExternalArtworkHttpResponse& response)
{
    if (response.transportError) return "provider transport failed";
    if (response.statusCode == 401L || response.statusCode == 403L)
        return "provider credentials rejected";
    if (response.statusCode == 404L) return "provider resource not found";
    if (response.statusCode == 429L) return "provider rate limited";
    if (response.statusCode >= 500L) return "provider temporarily unavailable";
    if (response.statusCode != 200L) return "provider request failed";
    return "provider returned invalid JSON";
}
}

TmdbRecordingMetadataCandidateProvider::
TmdbRecordingMetadataCandidateProvider(
    IExternalArtworkHttpTransport& transport,
    TmdbRecordingMetadataCandidateProviderConfig config,
    Sleeper sleeper)
    : transport_(transport),
      config_(std::move(config)),
      sleeper_(sleeper ? std::move(sleeper) : Sleeper(
          [](std::chrono::milliseconds delay) {
              std::this_thread::sleep_for(delay);
          }))
{
}

bool TmdbRecordingMetadataCandidateProvider::configurationValid(
    const TmdbRecordingMetadataCandidateProviderConfig& config)
{
    return !config.readAccessToken.empty() &&
        config.readAccessToken.size() <= 4096U &&
        !controlOrWhitespace(config.readAccessToken) &&
        validLanguage(config.language) &&
        config.connectTimeoutMs >= 100 && config.connectTimeoutMs <= 10000 &&
        config.totalTimeoutMs >= config.connectTimeoutMs &&
        config.totalTimeoutMs <= 30000 &&
        config.maximumRetries >= 0 && config.maximumRetries <= 2 &&
        config.retryBackoffMs >= 50 && config.retryBackoffMs <= 2000 &&
        config.maximumJsonBytes >= 1024U &&
        config.maximumJsonBytes <= 2U * 1024U * 1024U;
}

RecordingMetadataCandidatePage
TmdbRecordingMetadataCandidateProvider::search(
    const std::string& query,
    RecordingMetadataCandidateKind kind,
    int limit)
{
    RecordingMetadataCandidatePage invalid;
    invalid.providerId = "tmdb";
    if (!configurationValid(config_) || !validQuery(query) ||
        (kind != RecordingMetadataCandidateKind::Movie &&
         kind != RecordingMetadataCandidateKind::Series) ||
        limit < 1 || limit > 20)
    {
        invalid.error = "invalid candidate search request";
        return invalid;
    }

    const std::string media =
        kind == RecordingMetadataCandidateKind::Movie ? "movie" : "tv";
    return request(
        std::string(ApiBase) + "/search/" + media +
            "?query=" + encodeQuery(query) +
            "&language=" + encodeQuery(config_.language) +
            "&include_adult=false&page=1",
        kind,
        "",
        0,
        limit);
}

RecordingMetadataCandidatePage
TmdbRecordingMetadataCandidateProvider::seasons(
    const std::string& seriesExternalId,
    int limit)
{
    RecordingMetadataCandidatePage invalid;
    invalid.providerId = "tmdb";
    if (!configurationValid(config_) || !digits(seriesExternalId) ||
        limit < 1 || limit > 20)
    {
        invalid.error = "invalid series seasons request";
        return invalid;
    }

    return request(
        std::string(ApiBase) + "/tv/" + seriesExternalId +
            "?language=" + encodeQuery(config_.language),
        RecordingMetadataCandidateKind::Season,
        seriesExternalId,
        0,
        limit);
}

RecordingMetadataCandidatePage
TmdbRecordingMetadataCandidateProvider::episodes(
    const std::string& seriesExternalId,
    int seasonNumber,
    int limit)
{
    RecordingMetadataCandidatePage invalid;
    invalid.providerId = "tmdb";
    if (!configurationValid(config_) || !digits(seriesExternalId) ||
        seasonNumber <= 0 || seasonNumber > 10000 ||
        limit < 1 || limit > 20)
    {
        invalid.error = "invalid series episodes request";
        return invalid;
    }

    return request(
        std::string(ApiBase) + "/tv/" + seriesExternalId +
            "/season/" + std::to_string(seasonNumber) +
            "?language=" + encodeQuery(config_.language),
        RecordingMetadataCandidateKind::Episode,
        seriesExternalId,
        seasonNumber,
        limit);
}

RecordingMetadataCandidatePage
TmdbRecordingMetadataCandidateProvider::request(
    const std::string& url,
    RecordingMetadataCandidateKind kind,
    const std::string& parentExternalId,
    int seasonNumber,
    int limit)
{
    RecordingMetadataCandidatePage page;
    page.attempted = true;
    page.providerId = "tmdb";

    ExternalArtworkHttpResponse response;
    for (int attempt = 0; attempt <= config_.maximumRetries; ++attempt)
    {
        ExternalArtworkHttpRequest request;
        request.url = url;
        request.bearerToken = config_.readAccessToken;
        request.accept = "application/json";
        request.connectTimeoutMs = config_.connectTimeoutMs;
        request.totalTimeoutMs = config_.totalTimeoutMs;
        request.maximumResponseBytes = config_.maximumJsonBytes;
        response = transport_.perform(request);
        if (!retryable(response) || attempt == config_.maximumRetries) break;

        long long delay = static_cast<long long>(config_.retryBackoffMs) << attempt;
        if (response.retryAfterSeconds > 0)
            delay = std::max(delay, response.retryAfterSeconds * 1000LL);
        sleeper_(std::chrono::milliseconds(std::min(delay, 2000LL)));
    }

    if (!jsonReady(response))
    {
        page.providerAvailable =
            !response.transportError && response.statusCode < 500L &&
            response.statusCode != 429L;
        page.error = responseError(response);
        return page;
    }

    bool parsed = false;
    if (kind == RecordingMetadataCandidateKind::Movie ||
        kind == RecordingMetadataCandidateKind::Series)
        parsed = parseTmdbRecordingCandidateSearch(
            response.body, config_.maximumJsonBytes, kind, limit,
            page.candidates, page.truncated);
    else if (kind == RecordingMetadataCandidateKind::Season)
        parsed = parseTmdbRecordingCandidateSeasons(
            response.body, config_.maximumJsonBytes, parentExternalId, limit,
            page.candidates, page.truncated);
    else if (kind == RecordingMetadataCandidateKind::Episode)
        parsed = parseTmdbRecordingCandidateEpisodes(
            response.body, config_.maximumJsonBytes, parentExternalId,
            seasonNumber, limit, page.candidates, page.truncated);

    page.providerAvailable = true;
    if (!parsed)
    {
        page.candidates.clear();
        page.error = "provider returned invalid JSON";
    }
    return page;
}
