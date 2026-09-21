#include "ChangeDetectionService.h"

#include <cassert>
#include <iostream>

int main()
{
    ChangeDetectionService service;

    VdrChangeState oldState;
    VdrChangeState newState;

    auto noChanges = service.detectChanges(oldState, newState);
    assert(noChanges.empty());

    newState.channelsVersion = 1;
    auto channelChanges = service.detectChanges(oldState, newState);
    assert(channelChanges.size() == 1);
    assert(channelChanges[0].type() == VdrChangeType::ChannelsChanged);

    newState.recordingsVersion = 1;
    auto multipleChanges = service.detectChanges(oldState, newState);
    assert(multipleChanges.size() == 2);

    VdrChangeState searchTimerState;
    searchTimerState.searchTimersVersion = 1;

    auto searchTimerChanges = service.detectChanges(oldState, searchTimerState);
    assert(searchTimerChanges.size() == 1);
    assert(searchTimerChanges[0].type() == VdrChangeType::SearchTimersChanged);
    assert(searchTimerChanges[0].typeName() == "SearchTimersChanged");

    std::cout << "test_change_detection_service passed" << std::endl;
    return 0;
}
