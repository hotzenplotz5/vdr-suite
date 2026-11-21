#include "ChangeDetectionService.h"

std::vector<VdrChangeEvent> ChangeDetectionService::detectChanges(
    const VdrChangeState& previous,
    const VdrChangeState& current) const
{
    std::vector<VdrChangeEvent> events;

    if (previous.statusVersion != current.statusVersion) {
        events.emplace_back(VdrChangeType::StatusChanged);
    }

    if (previous.channelsVersion != current.channelsVersion) {
        events.emplace_back(VdrChangeType::ChannelsChanged);
    }

    if (previous.recordingsVersion != current.recordingsVersion) {
        events.emplace_back(VdrChangeType::RecordingsChanged);
    }

    if (previous.timersVersion != current.timersVersion) {
        events.emplace_back(VdrChangeType::TimersChanged);
    }

    if (previous.searchTimersVersion != current.searchTimersVersion) {
        events.emplace_back(VdrChangeType::SearchTimersChanged);
    }

    if (previous.eventsVersion != current.eventsVersion) {
        events.emplace_back(VdrChangeType::EventsChanged);
    }

    return events;
}
