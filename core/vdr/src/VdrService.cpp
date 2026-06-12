#include "VdrService.h"

VdrService::VdrService(IVdrAdapter& adapter, IRuntimeLogger* logger)
    : adapter_(adapter),
      logger_(logger)
{
}

void VdrService::log(RuntimeLogLevel level, const std::string& message) const
{
    if (logger_ == nullptr) {
        return;
    }

    logger_->write(RuntimeLogEntry{level, "VdrService", message});
}

VdrStatus VdrService::getStatus() const
{
    log(RuntimeLogLevel::Info, "Loading status");
    return adapter_.getStatus();
}

std::vector<VdrChannel> VdrService::getChannels() const
{
    log(RuntimeLogLevel::Info, "Loading channels");
    return adapter_.getChannels();
}

std::vector<VdrEvent> VdrService::getEvents() const
{
    log(RuntimeLogLevel::Info, "Loading events");
    return adapter_.getEvents();
}

std::vector<VdrEvent> VdrService::getEvents(const VdrEventQuery& query) const
{
    log(RuntimeLogLevel::Info, "Loading events with query");
    return adapter_.getEvents(query);
}

std::vector<VdrTimer> VdrService::getTimers() const
{
    log(RuntimeLogLevel::Info, "Loading timers");
    return adapter_.getTimers();
}

std::vector<VdrRecording> VdrService::getRecordings() const
{
    log(RuntimeLogLevel::Info, "Loading recordings");
    return adapter_.getRecordings();
}

VdrChangeState VdrService::getChangeState() const
{
    log(RuntimeLogLevel::Info, "Loading change state");
    return adapter_.getChangeState();
}
