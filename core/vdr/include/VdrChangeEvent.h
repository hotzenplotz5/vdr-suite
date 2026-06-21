#ifndef VDR_CHANGE_EVENT_H
#define VDR_CHANGE_EVENT_H

#include <string>

enum class VdrChangeType {
    StatusChanged,
    ChannelsChanged,
    RecordingsChanged,
    TimersChanged,
    SearchTimersChanged,
    EventsChanged
};

class VdrChangeEvent {
public:
    explicit VdrChangeEvent(VdrChangeType type);

    VdrChangeType type() const;
    std::string typeName() const;

private:
    VdrChangeType type_;
};

#endif
