#include "VdrService.h"

VdrService::VdrService(IVdrAdapter& adapter)
    : adapter_(adapter)
{
}

VdrStatus VdrService::getStatus() const
{
    return adapter_.getStatus();
}

std::vector<VdrChannel> VdrService::getChannels() const
{
    return adapter_.getChannels();
}

std::vector<VdrEvent> VdrService::getEvents() const
{
    return adapter_.getEvents();
}

std::vector<VdrTimer> VdrService::getTimers() const
{
    return adapter_.getTimers();
}

std::vector<VdrRecording> VdrService::getRecordings() const
{
    return adapter_.getRecordings();
}

VdrChangeState VdrService::getChangeState() const
{
    return adapter_.getChangeState();
}
