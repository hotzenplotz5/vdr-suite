#include "RecordingActionUtils.h"

#include <iostream>

int main()
{
    auto action =
        RecordingActionType::Shrink;

    auto text =
        toString(action);

    std::cout << text << std::endl;

    auto parsed =
        fromString(text);

    std::cout
        << toString(parsed)
        << std::endl;

    return 0;
}
