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

    const std::string json = RecordingMediaTrackContract::json(
        source,
        1,
        true,
        "",
        false,
        "profile_does_not_deliver_selectable_subtitles");

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
    assert(json.find("\"format\":\"dvb-subtitle\"") != std::string::npos);
    assert(json.find("\"roles\":[\"forced\"]") != std::string::npos);
    assert(json.find("\"format\":\"teletext\"") != std::string::npos);
    assert(json.find("\"roles\":[\"hearing-impaired\"]") != std::string::npos);
    assert(json.find("\"offSupported\":true") != std::string::npos);
    assert(json.find("\"offSelected\":true") != std::string::npos);
    assert(json.find("\"selectionSupported\":false") != std::string::npos);
    assert(json.find("profile_does_not_deliver_selectable_subtitles") != std::string::npos);

    return 0;
}
