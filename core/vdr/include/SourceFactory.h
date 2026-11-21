#pragma once

#include "Source.h"

inline Source createLocalVdrSource()
{
    Source source;

    source.id = "local-vdr";
    source.name = "Local VDR";
    source.type = SourceType::LocalVdr;
    source.enabled = true;

    return source;
}
