#pragma once

#include "RecordingMetadataCandidateProvider.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

class IExternalArtworkHttpTransport;

struct TmdbRecordingMetadataCandidateProviderConfig
{
    std::string readAccessToken;
    std::string language = "de-DE";
    int connectTimeoutMs = 2000;
    int totalTimeoutMs = 8000;
    int maximumRetries = 1;
    int retryBackoffMs = 250;
    std::size_t maximumJsonBytes = 512U * 1024U;
};

class TmdbRecordingMetadataCandidateProvider final
    : public IRecordingMetadataCandidateProvider
{
public:
    using Sleeper = std::function<void(std::chrono::milliseconds)>;

    TmdbRecordingMetadataCandidateProvider(
        IExternalArtworkHttpTransport& transport,
        TmdbRecordingMetadataCandidateProviderConfig config,
        Sleeper sleeper = {});

    RecordingMetadataCandidatePage search(
        const std::string& query,
        RecordingMetadataCandidateKind kind,
        int limit) override;

    RecordingMetadataCandidatePage seasons(
        const std::string& seriesExternalId,
        int limit) override;

    RecordingMetadataCandidatePage episodes(
        const std::string& seriesExternalId,
        int seasonNumber,
        int limit) override;

    static bool configurationValid(
        const TmdbRecordingMetadataCandidateProviderConfig& config);

private:
    IExternalArtworkHttpTransport& transport_;
    TmdbRecordingMetadataCandidateProviderConfig config_;
    Sleeper sleeper_;

    RecordingMetadataCandidatePage request(
        const std::string& url,
        RecordingMetadataCandidateKind kind,
        const std::string& parentExternalId,
        int seasonNumber,
        int limit);
};
