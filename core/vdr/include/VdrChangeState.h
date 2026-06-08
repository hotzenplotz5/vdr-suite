#ifndef VDR_CHANGE_STATE_H
#define VDR_CHANGE_STATE_H

class VdrChangeState {
public:
    VdrChangeState();

    bool hasChangesComparedTo(const VdrChangeState& other) const;
    bool isEmpty() const;

    int statusVersion;
    int channelsVersion;
    int recordingsVersion;
    int timersVersion;
    int eventsVersion;
};

#endif
