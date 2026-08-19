#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

struct FfmpegLiveStreamCommandPlan
{
    bool valid = false;
    std::string reasonCode;
    std::vector<std::string> argv;
};

class FfmpegLiveStreamCommandBuilder
{
public:
    FfmpegLiveStreamCommandPlan build(
        const std::string& unixSocketPath,
        const std::string& outputPath) const
    {
        FfmpegLiveStreamCommandPlan plan;
        if (!validAbsolutePath(unixSocketPath, 100)) {
            plan.reasonCode = "invalid_live_source_socket";
            return plan;
        }
        if (!validAbsolutePath(outputPath, 512)) {
            plan.reasonCode = "invalid_live_stream_output";
            return plan;
        }

        // VNSI-style hot path: exactly one consumer of the conditioned native
        // TS. Video stays H.264 bit-exact; only browser-incompatible audio is
        // adapted to AAC before fragmented MP4 is emitted continuously.
        plan.argv = {
            "/usr/bin/ffmpeg",
            "-nostdin",
            "-hide_banner",
            "-loglevel", "warning",
            "-y",
            "-fflags", "+nobuffer",
            "-analyzeduration", "250000",
            "-probesize", "262144",
            "-rw_timeout", "5000000",
            "-f", "mpegts",
            "-i", "unix://" + unixSocketPath,
            "-map", "0:v:0?",
            "-map", "0:a:0?",
            "-sn",
            "-c:v", "copy",
            "-c:a", "aac",
            "-b:a", "192k",
            "-ac", "2",
            "-f", "mp4",
            "-movflags", "+empty_moov+default_base_moof+frag_keyframe+omit_tfhd_offset",
            "-frag_duration", "250000",
            "-min_frag_duration", "100000",
            "-flush_packets", "1",
            outputPath
        };
        plan.valid = true;
        return plan;
    }

private:
    static bool validAbsolutePath(
        const std::string& value,
        std::size_t maximumLength)
    {
        if (value.empty() || value.front() != '/' || value.size() > maximumLength ||
            value.find("..") != std::string::npos || value.find('?') != std::string::npos ||
            value.find('#') != std::string::npos) {
            return false;
        }
        return std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '/' ||
                character == '-' || character == '_' || character == '.' ||
                character == ':';
        });
    }
};
