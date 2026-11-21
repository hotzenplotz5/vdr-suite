#include "RecordingSourceFingerprint.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{

class TemporaryRecording
{
public:
    TemporaryRecording()
    {
        char pattern[] = "/tmp/vdr-suite-fingerprint-XXXXXX";
        char* created = ::mkdtemp(pattern);
        assert(created != nullptr);
        directory_ = created;
        segment_ = directory_ / "00001.ts";
        writeSegment("recording-data");
    }

    ~TemporaryRecording()
    {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    const std::string directory() const { return directory_.string(); }
    const std::vector<std::string> segments() const { return {segment_.string()}; }

    void setTimerMarker(bool present)
    {
        const std::filesystem::path marker = directory_ / ".timer";
        if (present) {
            std::ofstream output(marker, std::ios::binary | std::ios::trunc);
            assert(output.good());
            output << "0@yavdr\n";
            output.close();
            assert(output.good());
        }
        else {
            std::error_code error;
            std::filesystem::remove(marker, error);
            assert(!error);
        }
    }

    void appendSegment(const std::string& value)
    {
        std::ofstream output(segment_, std::ios::binary | std::ios::app);
        assert(output.good());
        output << value;
        output.close();
        assert(output.good());
    }

private:
    void writeSegment(const std::string& value)
    {
        std::ofstream output(segment_, std::ios::binary | std::ios::trunc);
        assert(output.good());
        output << value;
        output.close();
        assert(output.good());
    }

    std::filesystem::path directory_;
    std::filesystem::path segment_;
};

} // namespace

int main()
{
    TemporaryRecording recording;

    const RecordingSourceFingerprint completed =
        inspectRecordingSource(recording.directory(), recording.segments());
    assert(completed.valid);
    assert(!completed.growing);
    assert(completed.value.find("|growing=0") != std::string::npos);

    recording.setTimerMarker(true);
    const RecordingSourceFingerprint markerOnly =
        inspectRecordingSource(recording.directory(), recording.segments());
    assert(markerOnly.valid);
    assert(markerOnly.growing);
    assert(markerOnly.value.find("|growing=1") != std::string::npos);
    assert(sameRecordingSourceExtentIgnoringGrowthState(
        completed.value,
        markerOnly.value));

    recording.appendSegment("-changed");
    const RecordingSourceFingerprint changedWhileMarked =
        inspectRecordingSource(recording.directory(), recording.segments());
    assert(changedWhileMarked.valid);
    assert(changedWhileMarked.growing);
    assert(!sameRecordingSourceExtentIgnoringGrowthState(
        completed.value,
        changedWhileMarked.value));

    recording.setTimerMarker(false);
    const RecordingSourceFingerprint changedCompleted =
        inspectRecordingSource(recording.directory(), recording.segments());
    assert(changedCompleted.valid);
    assert(!changedCompleted.growing);
    assert(!sameRecordingSourceExtentIgnoringGrowthState(
        completed.value,
        changedCompleted.value));

    assert(!sameRecordingSourceExtentIgnoringGrowthState(
        "v1|directory=broken",
        markerOnly.value));
    assert(!sameRecordingSourceExtentIgnoringGrowthState(
        completed.value,
        "v2|directory=broken|growing=1"));

    std::cout
        << "recording source fingerprint distinguishes transient growth marker from extent changes ok\n";
    return 0;
}
