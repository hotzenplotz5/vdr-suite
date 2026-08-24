#include "RecordingMediaTrackContract.h"

#include <cctype>
#include <limits>

namespace
{

std::string jsonEscape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value) {
        switch (character) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (character < 0x20) {
                static constexpr char Hex[] = "0123456789abcdef";
                result += "\\u00";
                result.push_back(Hex[(character >> 4) & 0x0f]);
                result.push_back(Hex[character & 0x0f]);
            }
            else result.push_back(static_cast<char>(character));
        }
    }
    return result;
}

std::string nullableString(const std::string& value)
{
    return value.empty() ? "null" : "\"" + jsonEscape(value) + "\"";
}

const char* codecName(MediaCodec codec)
{
    switch (codec) {
    case MediaCodec::H264: return "h264";
    case MediaCodec::H265: return "h265";
    case MediaCodec::Mpeg2Video: return "mpeg2video";
    case MediaCodec::Aac: return "aac";
    case MediaCodec::Ac3: return "ac3";
    case MediaCodec::Eac3: return "eac3";
    case MediaCodec::Dts: return "dts";
    case MediaCodec::MpegAudio: return "mpeg-audio";
    case MediaCodec::None: return "none";
    case MediaCodec::Unknown: return "unknown";
    }
    return "unknown";
}

const char* subtitleFormatName(MediaSubtitleFormat format)
{
    switch (format) {
    case MediaSubtitleFormat::Dvb: return "dvb-subtitle";
    case MediaSubtitleFormat::Teletext: return "teletext";
    case MediaSubtitleFormat::WebVtt: return "webvtt";
    case MediaSubtitleFormat::SubRip: return "subrip";
    case MediaSubtitleFormat::Ass: return "ass";
    case MediaSubtitleFormat::MovText: return "mov-text";
    case MediaSubtitleFormat::Unknown: return "unknown";
    }
    return "unknown";
}

void appendRole(std::string& json, bool& first, bool enabled, const char* role)
{
    if (!enabled) return;
    if (!first) json += ',';
    first = false;
    json += "\"" + std::string(role) + "\"";
}

std::string audioRoles(const MediaAudioStreamDescriptor& track)
{
    std::string result = "[";
    bool first = true;
    appendRole(result, first, track.original, "original");
    appendRole(result, first, track.commentary, "commentary");
    appendRole(result, first, track.descriptive, "descriptive");
    appendRole(result, first, track.hearingImpaired, "hearing-impaired");
    result += ']';
    return result;
}

std::string subtitleRoles(const MediaSubtitleStreamDescriptor& track)
{
    std::string result = "[";
    bool first = true;
    appendRole(result, first, track.forced, "forced");
    appendRole(result, first, track.hearingImpaired, "hearing-impaired");
    result += ']';
    return result;
}

std::string defaultAudioTrackId(const MediaSourceDescriptor& source)
{
    for (std::size_t index = 0; index < source.audioStreams.size(); ++index) {
        if (source.audioStreams[index].defaultTrack) {
            return RecordingMediaTrackContract::audioTrackId(index);
        }
    }
    return {};
}

std::string defaultSubtitleTrackId(const MediaSourceDescriptor& source)
{
    for (std::size_t index = 0; index < source.subtitleStreams.size(); ++index) {
        if (source.subtitleStreams[index].defaultTrack) {
            return RecordingMediaTrackContract::subtitleTrackId(index);
        }
    }
    return {};
}

} // namespace

std::string RecordingMediaTrackContract::audioTrackId(
    std::size_t sourceAudioStreamIndex)
{
    return "audio-" + std::to_string(sourceAudioStreamIndex + 1);
}

std::string RecordingMediaTrackContract::subtitleTrackId(
    std::size_t sourceSubtitleStreamIndex)
{
    return "subtitle-" + std::to_string(sourceSubtitleStreamIndex + 1);
}

bool RecordingMediaTrackContract::audioStreamIndexForTrackId(
    const std::string& trackId,
    const MediaSourceDescriptor& source,
    int& sourceAudioStreamIndex)
{
    sourceAudioStreamIndex = -1;
    constexpr const char* Prefix = "audio-";
    if (trackId.rfind(Prefix, 0) != 0 || trackId.size() <= 6 || trackId.size() > 16) {
        return false;
    }
    unsigned long long value = 0;
    for (std::size_t index = 6; index < trackId.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(trackId[index]);
        if (!std::isdigit(character)) return false;
        const unsigned digit = static_cast<unsigned>(character - '0');
        if (value > (std::numeric_limits<unsigned long long>::max() - digit) / 10ULL) {
            return false;
        }
        value = value * 10ULL + digit;
    }
    if (value == 0 || value > source.audioStreams.size() ||
        value - 1ULL > static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
        return false;
    }
    sourceAudioStreamIndex = static_cast<int>(value - 1ULL);
    return true;
}

std::string RecordingMediaTrackContract::json(
    const MediaSourceDescriptor& source,
    int selectedAudioStreamIndex,
    bool audioSelectionSupported,
    const std::string& audioSelectionReason,
    bool subtitleSelectionSupported,
    const std::string& subtitleSelectionReason,
    bool subtitleOffSupported,
    int subtitleOffSelectedState)
{
    std::string result = "{\"audio\":{";
    result += "\"selectionSupported\":";
    result += audioSelectionSupported ? "true" : "false";
    result += ",\"selectionReason\":" + nullableString(audioSelectionReason);
    result += ",\"availableTracks\":[";
    for (std::size_t index = 0; index < source.audioStreams.size(); ++index) {
        if (index > 0) result += ',';
        const auto& track = source.audioStreams[index];
        result += "{\"id\":\"" + audioTrackId(index) + "\",";
        result += "\"language\":" + nullableString(track.language) + ',';
        result += "\"label\":" + nullableString(track.label) + ',';
        result += "\"codec\":\"" + std::string(codecName(track.codec)) + "\",";
        result += "\"channels\":";
        result += track.channels > 0 ? std::to_string(track.channels) : "null";
        result += ",\"layout\":" + nullableString(track.channelLayout) + ',';
        result += "\"roles\":" + audioRoles(track) + ',';
        result += "\"default\":" + std::string(track.defaultTrack ? "true" : "false") + ',';
        result += "\"selected\":" + std::string(
            selectedAudioStreamIndex == static_cast<int>(index) ? "true" : "false") + '}';
    }
    result += "],\"selectedTrackId\":";
    if (selectedAudioStreamIndex >= 0 &&
        static_cast<std::size_t>(selectedAudioStreamIndex) < source.audioStreams.size()) {
        result += "\"" + audioTrackId(static_cast<std::size_t>(selectedAudioStreamIndex)) + "\"";
    }
    else result += "null";
    const std::string audioDefault = defaultAudioTrackId(source);
    result += ",\"defaultTrackId\":" + nullableString(audioDefault) + "},";

    result += "\"subtitles\":{";
    result += "\"selectionSupported\":";
    result += subtitleSelectionSupported ? "true" : "false";
    result += ",\"selectionReason\":" + nullableString(subtitleSelectionReason);
    result += ",\"offSupported\":" + std::string(subtitleOffSupported ? "true" : "false");
    result += ",\"offSelected\":";
    if (subtitleOffSelectedState < 0) result += "null";
    else result += subtitleOffSelectedState == 0 ? "false" : "true";
    result += ",\"availableTracks\":[";
    for (std::size_t index = 0; index < source.subtitleStreams.size(); ++index) {
        if (index > 0) result += ',';
        const auto& track = source.subtitleStreams[index];
        result += "{\"id\":\"" + subtitleTrackId(index) + "\",";
        result += "\"language\":" + nullableString(track.language) + ',';
        result += "\"label\":" + nullableString(track.label) + ',';
        result += "\"format\":\"" + std::string(subtitleFormatName(track.format)) + "\",";
        result += "\"roles\":" + subtitleRoles(track) + ',';
        result += "\"default\":" + std::string(track.defaultTrack ? "true" : "false") + ',';
        result += "\"forced\":" + std::string(track.forced ? "true" : "false") + '}';
    }
    result += "],\"selectedTrackId\":null";
    const std::string subtitleDefault = defaultSubtitleTrackId(source);
    result += ",\"defaultTrackId\":" + nullableString(subtitleDefault) + "}}";
    return result;
}
