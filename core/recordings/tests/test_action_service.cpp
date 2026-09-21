#include "ActionService.h"
#include "RecordingActionUtils.h"

#include <iostream>

int main()
{
    ActionService service;

    auto action =
        service.createAction(
            1,
            RecordingActionType::Shrink);

    std::cout
        << "Recording: "
        << action.recordingId
        << std::endl;

    std::cout
        << "Action: "
        << toString(action.type)
        << std::endl;

    std::cout
        << "Status: "
        << action.status
        << std::endl;

    return 0;
}
