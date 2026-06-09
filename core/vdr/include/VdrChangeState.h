#ifndef VDR_CHANGE_STATE_H
#define VDR_CHANGE_STATE_H

#include <string>

class VdrChangeState {
public:
    VdrChangeState();

    bool hasChangesComparedTo(const VdrChangeState& other) const;
    bool isEmpty() const;

    std::string bootId;
    long long statusVersion;
    long long channelsVersion;
    long long recordingsVersion;
    long long timersVersion;
    long long eventsVersion;
};

#endif
