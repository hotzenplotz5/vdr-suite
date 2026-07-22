#include "VdrRecordingMetadata.h"
#include "VdrRecordingNativeMetadata.h"

#include <cassert>

int main()
{
    VdrRecordingMetadata recordingMetadata;
    recordingMetadata.native.eventTitle = "Native VDR title";
    assert(recordingMetadata.native.hasText());

    VdrRecordingNativeMetadata resolvedMetadata;
    resolvedMetadata.availability =
        VdrRecordingNativeMetadataAvailability::Found;
    resolvedMetadata.title = "Resolved TVScraper title";

    assert(resolvedMetadata.availability ==
        VdrRecordingNativeMetadataAvailability::Found);
    assert(resolvedMetadata.title == "Resolved TVScraper title");

    return 0;
}
