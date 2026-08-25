#include "RecordingMediaTrackContract.h"

#include <cassert>
#include <string>

int main()
{
    MediaSourceDescriptor source;

    MediaAudioStreamDescriptor german;
    german.codec = MediaCodec::Ac3;
    german.channels = 6;
    german.language = "deu";
    german.label = "Deutsch Dolby Digital";
    german.channelLayout = "5.1(side)";
    german.defaultTrack = true;
    source.audioStreams.push_back(german);

    MediaAudioStreamDescriptor original;
    original.codec = MediaCodec::Aac;
    original.channels = 2;
    original.language = "eng";
    original.original = true;
    source.audioStreams.push_back(original);

    MediaAudioStreamDescriptor unknown;
    unknown.codec = MediaCodec::MpegAudio;
    unknown.channels = 0;
    unknown.commentary = true;
    source.audioStreams.push_back(unknown);

    MediaSubtitleStreamDescriptor forced;
    forced.format = MediaSubtitleFormat::Dvb;
    forced.language = "deu";
    forced.forced = true;
    source.subtitleStreams.push_back(forced);

    MediaSubtitleStreamDescriptor hearing;
    hearing.format = MediaSubtitleFormat::Teletext;
    hearing.hearingImpaired = true;
    hearing.defaultTrack = true;
    source.subtitleStreams.push_back(hearing);

    MediaSubtitleStreamDescriptor englishText;
    englishText.format = MediaSubtitleFormat::SubRip;
    englishText.language = "eng";
    englishText.label = "English CC";
    englishText.hearingImpaired = true;
    source.subtitleStreams.push_back(englishText);

    assert(RecordingMediaTrackContract::audioTrackId(0) == "audio-1");
    assert(RecordingMediaTrackContract::audioTrackId(2) == "audio-3");
    assert(RecordingMediaTrackContract::subtitleTrackId(1) == "subtitle-2");

    int sourceIndex = -1;
    assert(RecordingMediaTrackContract::audioStreamIndexForTrackId(
        "audio-2", source, sourceIndex));
    assert(sourceIndex == 1);
    assert(!RecordingMediaTrackContract::audioStreamIndexForTrackId(
        "audio-0", source, sourceIndex));
    assert(!RecordingMediaTrackContract::audioStreamIndexForTrackId(
        "audio-4", source, sourceIndex));
    assert(!RecordingMediaTrackContract::audioStreamIndexForTrackId(
        "pid-1234", source, sourceIndex));

    assert(RecordingMediaTrackContract::subtitleStreamIndexForTrackId(
        "subtitle-3", source, sourceIndex));
    assert(sourceIndex == 2);
    assert(!RecordingMediaTrackContract::subtitleStreamIndexForTrackId(
        "subtitle-0", source, sourceIndex));
    assert(!RecordingMediaTrackContract::subtitleStreamIndexForTrackId(
        "subtitle-4", source, sourceIndex));
    assert(!RecordingMediaTrackContract::subtitleStreamIndexForTrackId(
        "spid-1234", source, sourceIndex));

    assert(!RecordingMediaTrackContract::subtitleTrackSelectable(MediaSubtitleFormat::Dvb));
    assert(!RecordingMediaTrackContract::subtitleTrackSelectable(MediaSubtitleFormat::Teletext));
    assert(RecordingMediaTrackContract::subtitleTrackSelectable(MediaSubtitleFormat::WebVtt));
    assert(RecordingMediaTrackContract::subtitleTrackSelectable(MediaSubtitleFormat::SubRip));
    assert(RecordingMediaTrackContract::subtitleTrackSelectable(MediaSubtitleFormat::Ass));
    assert(RecordingMediaTrackContract::subtitleTrackSelectable(MediaSubtitleFormat::MovText));

    const std::string json = RecordingMediaTrackContract::json(
        source,
        1,
        true,
        "",
        true,
        "",
        true,
        0,
        2);

    assert(json.find("\"id\":\"audio-1\"") != std::string::npos);
    assert(json.find("\"id\":\"audio-2\"") != std::string::npos);
    assert(json.find("\"selectedTrackId\":\"audio-2\"") != std::string::npos);
    assert(json.find("\"defaultTrackId\":\"audio-1\"") != std::string::npos);
    assert(json.find("\"language\":\"deu\"") != std::string::npos);
    assert(json.find("\"codec\":\"ac3\"") != std::string::npos);
    assert(json.find("\"channels\":6") != std::string::npos);
    assert(json.find("\"layout\":\"5.1(side)\"") != std::string::npos);
    assert(json.find("\"roles\":[\"original\"]") != std::string::npos);
    assert(json.find("\"language\":null") != std::string::npos);
    assert(json.find("\"channels\":null") != std::string::npos);
    assert(json.find("\"roles\":[\"commentary\"]") != std::string::npos);

    assert(json.find("\"id\":\"subtitle-1\"") != std::string::npos);
    assert(json.find("\"format\":\"dvb-subtitle\",\"selectable\":false,\"deliveryFormat\":null") != std::string::npos);
    assert(json.find("\"roles\":[\"forced\"]") != std::string::npos);
    assert(json.find("\"format\":\"teletext\",\"selectable\":false,\"deliveryFormat\":null") != std::string::npos);
    assert(json.find("\"roles\":[\"hearing-impaired\"]") != std::string::npos);
    assert(json.find("\"format\":\"subrip\",\"selectable\":true,\"deliveryFormat\":\"webvtt\"") != std::string::npos);
    assert(json.find("\"selectedTrackId\":\"subtitle-3\"") != std::string::npos);
    assert(json.find("\"offSupported\":true") != std::string::npos);
    assert(json.find("\"offSelected\":false") != std::string::npos);
    assert(json.find("\"selectionSupported\":true") != std::string::npos);

    const std::string directJson = RecordingMediaTrackContract::json(
        source,
        -1,
        false,
        "recording_audio_track_selection_profile_not_supported",
        false,
        "profile_does_not_deliver_selectable_subtitles",
        false,
        -1);
    assert(directJson.find("\"selectedTrackId\":null") != std::string::npos);
    assert(directJson.find("\"offSupported\":false") != std::string::npos);
    assert(directJson.find("\"offSelected\":null") != std::string::npos);

    return 0;
}
