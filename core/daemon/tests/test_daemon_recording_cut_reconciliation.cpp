#include "DaemonRecordingCutReconciliation.h"

#include <cassert>
#include <string>

int main()
{
    const std::string source = "0123456789abcdef0123456789abcdef";
    const std::string edited = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    VdrRecordingNativeCutState state;
    state.availability = VdrRecordingNativeCutStateAvailability::Available;
    state.found = true;
    state.recordingKey = source;
    state.reason = "edited-destination-exists";
    state.editedRecordingKey = edited;
    state.editedDestinationExists = true;
    state.editedRecordingFound = true;
    assert(daemonRecordingCutResultMatches(source, edited, state));

    state.editedRecordingFound = false;
    assert(!daemonRecordingCutResultMatches(source, edited, state));
    state.editedRecordingFound = true;

    state.editedRecordingKey = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    assert(!daemonRecordingCutResultMatches(source, edited, state));
    state.editedRecordingKey = edited;

    state.recordingKey = "cccccccccccccccccccccccccccccccc";
    assert(!daemonRecordingCutResultMatches(source, edited, state));
    state.recordingKey = source;

    state.availability = VdrRecordingNativeCutStateAvailability::TransportError;
    assert(!daemonRecordingCutResultMatches(source, edited, state));
    state.availability = VdrRecordingNativeCutStateAvailability::Available;

    state.editedDestinationExists = false;
    assert(!daemonRecordingCutResultMatches(source, edited, state));

    assert(!daemonRecordingCutResultMatches("/srv/vdr/video/source.rec", edited, state));
    assert(!daemonRecordingCutResultMatches(source, "/srv/vdr/video/edited.rec", state));

    return 0;
}
