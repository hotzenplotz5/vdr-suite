#include "ExternalVdrAdapter.h"

ExternalVdrAdapter::ExternalVdrAdapter(VdrConfig config)
    : config_(config)
{
}

VdrStatus ExternalVdrAdapter::getStatus() const
{
    VdrStatus status;
    status.enabled = config_.enabled;
    status.mode = config_.mode;
    status.host = config_.host;
    status.port = config_.port;
    status.state = config_.enabled ? "configured" : "disabled";

    return status;
}

std::vector<VdrEvent> ExternalVdrAdapter::getEvents() const
{
    return {};
}

std::vector<VdrEvent> ExternalVdrAdapter::getEvents(const VdrEventQuery& query) const
{
    (void)query;
    return {};
}

std::vector<VdrChannel> ExternalVdrAdapter::getChannels() const
{
    return {};
}

std::vector<VdrTimer> ExternalVdrAdapter::getTimers() const
{
    return {};
}

VdrTimerConflictReport ExternalVdrAdapter::getTimerConflictReport() const
{
    VdrTimerConflictReport report;
    report.source = "external";
    report.available = false;
    report.error = "timer conflict report unavailable for external adapter";

    return report;
}

std::vector<VdrRecording> ExternalVdrAdapter::getRecordings() const
{
    return {};
}

VdrChangeState ExternalVdrAdapter::getChangeState() const
{
    VdrChangeState state;
    return state;
}
