#pragma once

#include "MediaCheck.h"
#include "MediaCheckFfmpeg.h"
#include "MediaCheckApiRuntime.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>

// A single bounded, read-only, backend-local Media Check. No generic worker
// platform, filesystem mutation, browser-supplied paths or CLI shell commands.
// Until Agent transport is integrated this is ONLY enabled for explicit local
// backend video-root mounts. Its status is in-memory, not a durable Operation.
class DaemonMediaCheckRuntime final
{
public:
    using ResolveRecording = std::function<std::optional<std::filesystem::path>(
        const std::string&, const std::string&)>;
    using BackendAvailable = std::function<bool(const std::string&)>;

    DaemonMediaCheckRuntime(
        std::map<std::string, std::string> videoRoots,
        ResolveRecording resolve, BackendAvailable available)
        : videoRoots_(std::move(videoRoots)),
          resolve_(std::move(resolve)),
          available_(std::move(available)) {}

    ~DaemonMediaCheckRuntime()
    {
        if (worker_.joinable()) worker_.join();
    }

    DaemonMediaCheckRuntime(const DaemonMediaCheckRuntime&) = delete;
    DaemonMediaCheckRuntime& operator=(const DaemonMediaCheckRuntime&) = delete;

    ApiResponse capability(const std::string& backend) const
    {
        const bool enabled = videoRoots_.count(backend) &&
            available_ && available_(backend);
        return MediaCheckApiRuntime::json(200,
            "{\"backendId\":\"" + backend +
            "\",\"capability\":\"addons.media.check\",\"available\":" +
            (enabled ? "true" : "false") + "}");
    }

    ApiResponse start(const std::string& actor, const std::string& backend,
                      const std::string& publicId, const std::string& mode)
    {
        const auto root = videoRoots_.find(backend);
        if (root == videoRoots_.end() || !available_ ||
            !available_(backend))
            return MediaCheckApiRuntime::problem(503,
                "Media Check is not available on this local backend.");
        if (!resolve_)
            return MediaCheckApiRuntime::problem(503,
                "Recording resolver is not available.");
        const auto recording = resolve_(backend, publicId);
        if (!recording)
            return MediaCheckApiRuntime::problem(404,
                "Recording not found in the current backend snapshot.");

        namespace fs = std::filesystem;
        const fs::path videoRoot(root->second);
        const fs::path absolute(*recording);
        // Never trust a browser pathname. The root and recording are supplied
        // only by administrator configuration and the live VDR snapshot.
        if (!videoRoot.is_absolute() ||
            videoRoot.lexically_normal() != videoRoot ||
            !absolute.is_absolute() ||
            absolute.lexically_normal() != absolute)
            return MediaCheckApiRuntime::problem(503,
                "Backend recording location is not valid.");
        const fs::path relative = absolute.lexically_relative(videoRoot);
        if (relative.empty() || relative.is_absolute() ||
            relative.extension() != ".rec" ||
            std::any_of(relative.begin(), relative.end(),
                [](const fs::path& p) { return p == "." || p == ".."; }))
            return MediaCheckApiRuntime::problem(503,
                "Recording is outside the configured local video root.");

        std::lock_guard<std::mutex> lock(mutex_);
        if (job_.running)
            return MediaCheckApiRuntime::problem(409,
                "A Media Check is already running.");
        // Completed thread no longer needs the mutex, so join is bounded.
        if (worker_.joinable()) worker_.join();

        std::string id;
        try {
            std::random_device random;
            constexpr char hex[] = "0123456789abcdef";
            id = "chk_";
            for (int i = 0; i != 16; ++i) {
                const unsigned char byte = static_cast<unsigned char>(random());
                id += hex[byte >> 4];
                id += hex[byte & 15];
            }
            job_ = {id, actor, backend, publicId, mode, true, std::string()};
            worker_ = std::thread([this, videoRoot, relative, mode, publicId]() {
                std::string serialized;
                try {
                    vdrsuite::mediatools::MediaCheckReport report;
                    vdrsuite::mediatools::FfprobeMediaCheckProber prober;
                    if (mode == "decode") {
                        vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder;
                        report = vdrsuite::mediatools::checkRecordingWithProber(
                            videoRoot, relative, prober, &decoder);
                    } else {
                        report = vdrsuite::mediatools::checkRecordingWithProber(
                            videoRoot, relative, prober);
                    }
                    // Public reports expose the opaque identity, never a path.
                    report.recording = publicId;
                    serialized = report.json();
                } catch (...) {
                    serialized =
                        "{\"schemaVersion\":1,\"status\":\"incomplete\","
                        "\"readOnly\":true,\"diagnostic\":"
                        "\"The recording could not be fully inspected.\"}";
                }
                std::lock_guard<std::mutex> update(mutex_);
                job_.report = std::move(serialized);
                job_.running = false;
            });
        } catch (...) {
            job_ = {};
            return MediaCheckApiRuntime::problem(503,
                "Cannot start Media Check.");
        }
        return MediaCheckApiRuntime::json(202, snapshotJson(job_));
    }

    ApiResponse status(const std::string& actor, const std::string& backend,
                       const std::string& checkId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (job_.id != checkId || job_.actor != actor ||
            job_.backend != backend)
            return MediaCheckApiRuntime::problem(404,
                "Media Check not found.");
        return MediaCheckApiRuntime::json(200, snapshotJson(job_));
    }

private:
    struct Job {
        std::string id, actor, backend, recordingId, mode;
        bool running = false;
        std::string report;
    };
    const std::map<std::string, std::string> videoRoots_;
    const ResolveRecording resolve_;
    const BackendAvailable available_;
    mutable std::mutex mutex_;
    Job job_;
    std::thread worker_;

    static std::string snapshotJson(const Job& job)
    {
        // Identifiers and mode are restricted to safe ASCII by the API gate
        // and the canonical PublicRecordingIdentityRepository.
        return "{\"checkId\":\"" + job.id + "\",\"backendId\":\"" +
            job.backend + "\",\"recordingId\":\"" + job.recordingId +
            "\",\"mode\":\"" + job.mode + "\",\"state\":\"" +
            (job.running ? "running" : "completed") + "\",\"report\":" +
            (job.running ? "null" : job.report) + "}";
    }
};
