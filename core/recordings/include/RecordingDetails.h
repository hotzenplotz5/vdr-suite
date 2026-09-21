#pragma once

#include "Recording.h"
#include "Metadata.h"

struct RecordingDetails
{
    Recording recording;
    Metadata metadata;

    bool hasMetadata = false;
};
