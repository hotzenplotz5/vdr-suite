#include "RecordingSubtitleSidecar.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

int main()
{
    char templatePath[] = "/tmp/vdr-suite-subtitle-sidecar-XXXXXX";
    char* created = ::mkdtemp(templatePath);
    assert(created != nullptr);
    const std::filesystem::path directory(created);
    const std::filesystem::path sidecar = directory / "00001.srt";

    const auto relative = RecordingSubtitleSidecar::discover("relative/path");
    assert(!relative.available);
    assert(relative.reasonCode == "invalid_recording_subtitle_sidecar_directory");

    const auto missing = RecordingSubtitleSidecar::discover(directory.string());
    assert(!missing.available);
    assert(missing.reasonCode == "recording_subtitle_sidecar_not_found");

    {
        std::ofstream output(sidecar);
    }
    const auto empty = RecordingSubtitleSidecar::discover(directory.string());
    assert(!empty.available);
    assert(empty.reasonCode == "recording_subtitle_sidecar_empty");

    {
        std::ofstream output(sidecar);
        output << "1\n00:00:01,000 --> 00:00:02,000\nHallo\n";
    }
    const auto regular = RecordingSubtitleSidecar::discover(directory.string());
    assert(regular.available);
    assert(regular.reasonCode.empty());
    assert(regular.path == sidecar.string());
    assert(regular.track.format == MediaSubtitleFormat::SubRip);
    assert(regular.track.label == "SRT");
    assert(regular.track.externalSourcePath == sidecar.string());

    MediaSourceDescriptor source;
    MediaSubtitleStreamDescriptor embedded;
    embedded.format = MediaSubtitleFormat::Dvb;
    source.subtitleStreams.push_back(embedded);
    assert(RecordingSubtitleSidecar::appendTo(source, directory.string()));
    assert(source.subtitleStreams.size() == 2);
    assert(source.subtitleStreams[0].format == MediaSubtitleFormat::Dvb);
    assert(source.subtitleStreams[1].format == MediaSubtitleFormat::SubRip);
    assert(source.subtitleStreams[1].externalSourcePath == sidecar.string());
    assert(RecordingSubtitleSidecar::appendTo(source, directory.string()));
    assert(source.subtitleStreams.size() == 2);

    std::filesystem::remove(sidecar);
    const std::filesystem::path target = directory / "outside.srt";
    {
        std::ofstream output(target);
        output << "1\n00:00:01,000 --> 00:00:02,000\nOutside\n";
    }
    std::filesystem::create_symlink(target, sidecar);
    const auto symlink = RecordingSubtitleSidecar::discover(directory.string());
    assert(!symlink.available);
    assert(symlink.reasonCode == "recording_subtitle_sidecar_not_regular_file");

    std::filesystem::remove_all(directory);
    return 0;
}
