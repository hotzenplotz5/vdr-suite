#include "VdrChangeEvent.h"

VdrChangeEvent::VdrChangeEvent(VdrChangeType type)
    : type_(type)
{
}

VdrChangeType VdrChangeEvent::type() const
{
    return type_;
}

std::string VdrChangeEvent::typeName() const
{
    switch (type_) {
    case VdrChangeType::StatusChanged:
        return "StatusChanged";
    case VdrChangeType::ChannelsChanged:
        return "ChannelsChanged";
    case VdrChangeType::RecordingsChanged:
        return "RecordingsChanged";
    case VdrChangeType::TimersChanged:
        return "TimersChanged";
    case VdrChangeType::SearchTimersChanged:
        return "SearchTimersChanged";
    case VdrChangeType::EventsChanged:
        return "EventsChanged";
    }

    return "Unknown";
}
