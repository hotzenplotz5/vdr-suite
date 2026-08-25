#include "RecordingSubtitleWebVtt.h"

#include <algorithm>
#include <cassert>
#include <string>

namespace
{

bool containsPair(
    const std::vector<std::string>& argv,
    const std::string& option,
    const std::string& value)
{
    for (std::size_t index = 0; index + 1 < argv.size(); ++index) {
        if (argv[index] == option && argv[index + 1] == value) return true;
    }
    return false;
}

} // namespace

int main()
{
    assert(RecordingSubtitleWebVtt::supports(MediaSubtitleFormat::WebVtt));
    assert(RecordingSubtitleWebVtt::supports(MediaSubtitleFormat::SubRip));
    assert(RecordingSubtitleWebVtt::supports(MediaSubtitleFormat::Ass));
    assert(RecordingSubtitleWebVtt::supports(MediaSubtitleFormat::MovText));
    assert(!RecordingSubtitleWebVtt::supports(MediaSubtitleFormat::Dvb));
    assert(!RecordingSubtitleWebVtt::supports(MediaSubtitleFormat::Teletext));
    assert(!RecordingSubtitleWebVtt::supports(MediaSubtitleFormat::Unknown));

    const auto valid = RecordingSubtitleWebVtt::build(2, MediaSubtitleFormat::SubRip);
    assert(valid.valid);
    assert(valid.reasonCode.empty());
    assert(!valid.argv.empty());
    assert(valid.argv.front() == "/usr/bin/ffmpeg");
    assert(containsPair(valid.argv, "-f", "concat"));
    assert(containsPair(valid.argv, "-i", "input.ffconcat"));
    assert(containsPair(valid.argv, "-map", "0:s:2"));
    assert(containsPair(valid.argv, "-c:s", "webvtt"));
    assert(valid.argv.back() == "pipe:1");

    const auto bitmap = RecordingSubtitleWebVtt::build(0, MediaSubtitleFormat::Dvb);
    assert(!bitmap.valid);
    assert(bitmap.reasonCode == "recording_subtitle_format_not_webvtt_convertible");

    const auto teletext = RecordingSubtitleWebVtt::build(0, MediaSubtitleFormat::Teletext);
    assert(!teletext.valid);
    assert(teletext.reasonCode == "recording_subtitle_format_not_webvtt_convertible");

    const auto negative = RecordingSubtitleWebVtt::build(-1, MediaSubtitleFormat::SubRip);
    assert(!negative.valid);
    assert(negative.reasonCode == "invalid_recording_subtitle_stream_index");

    return 0;
}
