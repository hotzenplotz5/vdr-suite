#include "FfprobeRecordingSource.h"

#include <cerrno>
#include <climits>
#include <cctype>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

namespace
{

MediaCodec codecFromName(const std::string& name)
{
    if (name == "h264") return MediaCodec::H264;
    if (name == "hevc" || name == "h265") return MediaCodec::H265;
    if (name == "mpeg2video") return MediaCodec::Mpeg2Video;
    if (name == "aac") return MediaCodec::Aac;
    if (name == "ac3") return MediaCodec::Ac3;
    if (name == "eac3") return MediaCodec::Eac3;
    if (name == "dts") return MediaCodec::Dts;
    if (name == "mp2" || name == "mp3") return MediaCodec::MpegAudio;
    return MediaCodec::Unknown;
}

MediaSubtitleFormat subtitleFormatFromName(const std::string& name)
{
    if (name == "dvb_subtitle") return MediaSubtitleFormat::Dvb;
    if (name == "dvb_teletext") return MediaSubtitleFormat::Teletext;
    if (name == "webvtt") return MediaSubtitleFormat::WebVtt;
    if (name == "subrip" || name == "srt") return MediaSubtitleFormat::SubRip;
    if (name == "ass" || name == "ssa") return MediaSubtitleFormat::Ass;
    if (name == "mov_text") return MediaSubtitleFormat::MovText;
    return MediaSubtitleFormat::Unknown;
}

bool interlacedFieldOrder(const std::string& value)
{
    return value == "tt" || value == "bb" ||
        value == "tb" || value == "bt";
}

int integerValue(const std::map<std::string, std::string>& fields, const std::string& key)
{
    const auto iterator = fields.find(key);
    if (iterator == fields.end() || iterator->second.empty()) return 0;

    char* end = nullptr;
    const long value = std::strtol(iterator->second.c_str(), &end, 10);
    if (end == iterator->second.c_str() || *end != '\0' || value < 0) return 0;
    return static_cast<int>(value);
}

long long nonNegativeBitRate(const std::map<std::string, std::string>& fields)
{
    const auto iterator = fields.find("bit_rate");
    if (iterator == fields.end() || iterator->second.empty()) return 0;
    errno = 0;
    char* end = nullptr;
    const long long number = std::strtoll(iterator->second.c_str(), &end, 10);
    return errno == 0 && end != iterator->second.c_str() &&
        *end == '\0' && number >= 0 ? number : 0;
}

double frameRateValue(const std::string& value)
{
    if (value.empty()) return 0.0;

    const std::size_t slash = value.find('/');
    if (slash == std::string::npos) {
        char* end = nullptr;
        const double parsed = std::strtod(value.c_str(), &end);
        return end != value.c_str() && *end == '\0' && parsed >= 0.0
            ? parsed
            : 0.0;
    }

    const std::string numeratorText = value.substr(0, slash);
    const std::string denominatorText = value.substr(slash + 1);
    char* numeratorEnd = nullptr;
    char* denominatorEnd = nullptr;
    const double numerator = std::strtod(numeratorText.c_str(), &numeratorEnd);
    const double denominator = std::strtod(denominatorText.c_str(), &denominatorEnd);

    if (numeratorEnd == numeratorText.c_str() || *numeratorEnd != '\0' ||
        denominatorEnd == denominatorText.c_str() || *denominatorEnd != '\0' ||
        numerator < 0.0 || denominator <= 0.0) return 0.0;

    return numerator / denominator;
}

std::map<std::string, std::string> parseCompactLine(const std::string& line)
{
    std::map<std::string, std::string> fields;
    std::size_t position = 0;

    while (position <= line.size()) {
        const std::size_t separator = line.find('|', position);
        const std::string token = separator == std::string::npos
            ? line.substr(position)
            : line.substr(position, separator - position);
        const std::size_t equals = token.find('=');
        if (equals != std::string::npos && equals > 0) {
            fields[token.substr(0, equals)] = token.substr(equals + 1);
        }
        if (separator == std::string::npos) break;
        position = separator + 1;
    }
    return fields;
}

std::string fieldValue(
    const std::map<std::string, std::string>& fields,
    const std::string& key)
{
    const auto iterator = fields.find(key);
    return iterator == fields.end() ? std::string{} : iterator->second;
}

bool flagValue(
    const std::map<std::string, std::string>& fields,
    const std::string& key)
{
    return fieldValue(fields, key) == "1";
}

std::string boundedCodecProfile(const std::string& profile)
{
    if (profile.empty() || profile.size() > 32U) return {};
    for (unsigned char c : profile) {
        if (!std::isalnum(c) && c != ' ' && c != '-' && c != '_' &&
            c != '.') return {};
    }
    return profile;
}

int knownPixelBitDepth(const std::string& pixelFormat)
{
    if (pixelFormat == "yuv420p" || pixelFormat == "yuv422p" ||
        pixelFormat == "yuv444p" || pixelFormat == "nv12") return 8;
    if (pixelFormat == "yuv420p10le" || pixelFormat == "yuv420p10be" ||
        pixelFormat == "yuv422p10le" || pixelFormat == "p010le") return 10;
    if (pixelFormat == "yuv420p12le" || pixelFormat == "yuv422p12le") return 12;
    return 0;
}

} // namespace

FfprobeRecordingPlan FfprobeRecordingSource::commandPlan() const
{
    FfprobeRecordingPlan plan;
    plan.argv = {
        "/usr/bin/ffprobe",
        "-v", "error",
        "-f", "concat",
        "-safe", "1",
        "-i", "input.ffconcat",
        "-show_entries",
        "stream=codec_type,codec_name,profile,level,bits_per_raw_sample,width,height,r_frame_rate,field_order,color_primaries,color_transfer,color_space,pix_fmt,time_base,bit_rate,channels,channel_layout:stream_tags=language,title:stream_disposition=default,original,comment,hearing_impaired,visual_impaired,forced",
        "-of", "compact=p=0:nk=0"
    };
    return plan;
}

FfprobeRecordingResult FfprobeRecordingSource::parse(
    const std::string& output) const
{
    FfprobeRecordingResult result;
    result.source.resourceKind = MediaResourceKind::Recording;
    result.source.container = MediaContainer::MpegTs;
    result.source.seekable = true;
    result.source.growing = false;

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        const auto fields = parseCompactLine(line);
        const std::string codecType = fieldValue(fields, "codec_type");
        const std::string codecName = fieldValue(fields, "codec_name");

        if (codecType == "video") {
            MediaVideoStreamDescriptor video;
            video.codec = codecFromName(codecName);
            video.width = integerValue(fields, "width");
            video.height = integerValue(fields, "height");
            video.framesPerSecond = frameRateValue(fieldValue(fields, "r_frame_rate"));
            video.interlaced = interlacedFieldOrder(fieldValue(fields, "field_order"));
            video.colorPrimaries = fieldValue(fields, "color_primaries");
            video.colorTransfer = fieldValue(fields, "color_transfer");
            video.colorSpace = fieldValue(fields, "color_space");
            video.pixelFormat = fieldValue(fields, "pix_fmt");
            video.timeBase = fieldValue(fields, "time_base");
            video.bitRate = nonNegativeBitRate(fields);
            video.codecProfile = boundedCodecProfile(fieldValue(fields, "profile"));
            video.codecLevel = integerValue(fields, "level");
            if (video.codecLevel > 255) video.codecLevel = 0;
            video.bitDepth = knownPixelBitDepth(video.pixelFormat);
            const int reportedDepth = integerValue(fields, "bits_per_raw_sample");
            if (reportedDepth > 0 && reportedDepth <= 16 &&
                (video.bitDepth == 0 || video.bitDepth == reportedDepth))
                video.bitDepth = reportedDepth;
            else if (reportedDepth > 0 && reportedDepth != video.bitDepth)
                video.bitDepth = 0;
            // PQ and HLG are explicit HDR transfer characteristics; absent
            // or unknown values must not be promoted to HDR.
            video.hdr = video.colorTransfer == "smpte2084" ||
                video.colorTransfer == "arib-std-b67";
            result.source.videoStreams.push_back(video);
        }
        else if (codecType == "audio") {
            MediaAudioStreamDescriptor audio;
            audio.codec = codecFromName(codecName);
            audio.channels = integerValue(fields, "channels");
            audio.language = fieldValue(fields, "tag:language");
            audio.label = fieldValue(fields, "tag:title");
            audio.channelLayout = fieldValue(fields, "channel_layout");
            audio.defaultTrack = flagValue(fields, "disposition:default");
            audio.original = flagValue(fields, "disposition:original");
            audio.commentary = flagValue(fields, "disposition:comment");
            audio.descriptive = flagValue(fields, "disposition:visual_impaired");
            audio.hearingImpaired = flagValue(fields, "disposition:hearing_impaired");
            result.source.audioStreams.push_back(audio);
        }
        else if (codecType == "subtitle") {
            MediaSubtitleStreamDescriptor subtitle;
            subtitle.format = subtitleFormatFromName(codecName);
            subtitle.language = fieldValue(fields, "tag:language");
            subtitle.label = fieldValue(fields, "tag:title");
            subtitle.defaultTrack = flagValue(fields, "disposition:default");
            subtitle.forced = flagValue(fields, "disposition:forced");
            subtitle.hearingImpaired = flagValue(fields, "disposition:hearing_impaired");
            result.source.subtitleStreams.push_back(subtitle);
        }
    }

    if (result.source.videoStreams.empty() && result.source.audioStreams.empty()) {
        result.reasonCode = "no_media_streams_detected";
        return result;
    }

    for (const auto& video : result.source.videoStreams) {
        if (video.codec == MediaCodec::Unknown) {
            result.reasonCode = "unknown_video_codec";
            return result;
        }
    }

    bool hasKnownAudioCodec = false;
    bool hasUnknownAudioCodec = false;
    for (const auto& audio : result.source.audioStreams) {
        if (audio.codec == MediaCodec::Unknown) hasUnknownAudioCodec = true;
        else if (audio.codec != MediaCodec::None) hasKnownAudioCodec = true;
    }

    if (hasUnknownAudioCodec && !hasKnownAudioCodec) {
        result.reasonCode = "unknown_audio_codec";
        return result;
    }

    result.valid = true;
    return result;
}
