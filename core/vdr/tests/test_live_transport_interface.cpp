#include "ILiveTransport.h"
#include "LiveUpdateEvent.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class CapturingLiveTransport : public ILiveTransport {
public:
    void publish(
        const LiveUpdateEvent& event) override
    {
        published_ = true;
        sequenceNumber_ = event.sequenceNumber();
        snapshotGeneration_ = event.snapshotGeneration();
        backendId_ = event.backendId();
        changedDomains_ = event.changedDomains();
    }

    bool published() const
    {
        return published_;
    }

    int sequenceNumber() const
    {
        return sequenceNumber_;
    }

    int snapshotGeneration() const
    {
        return snapshotGeneration_;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    const std::vector<std::string>& changedDomains() const
    {
        return changedDomains_;
    }

    std::string stream() const override
    {
        std::ostringstream output;
        output << sequenceNumber_ << ":" << backendId_;
        return output.str();
    }

    bool empty() const override
    {
        return !published_;
    }

    void clear() override
    {
        published_ = false;
        sequenceNumber_ = 0;
        snapshotGeneration_ = 0;
        backendId_.clear();
        changedDomains_.clear();
    }

private:
    bool published_ = false;
    int sequenceNumber_ = 0;
    int snapshotGeneration_ = 0;
    std::string backendId_;
    std::vector<std::string> changedDomains_;
};

static void test_live_transport_interface_accepts_live_update_event()
{
    CapturingLiveTransport transport;

    LiveUpdateEvent event(
        7,
        42,
        {"channels", "recordings"},
        "home-vdr");

    transport.publish(event);

    assert(transport.published() == true);
    assert(transport.sequenceNumber() == 7);
    assert(transport.snapshotGeneration() == 42);
    assert(transport.backendId() == "home-vdr");
    assert(transport.changedDomains().size() == 2);
    assert(transport.changedDomains()[0] == "channels");
    assert(transport.changedDomains()[1] == "recordings");
}


static void test_live_transport_interface_exposes_read_contract()
{
    CapturingLiveTransport transport;

    assert(transport.empty() == true);

    LiveUpdateEvent event(
        8,
        43,
        {"timers"},
        "ferienhaus-vdr");

    transport.publish(event);

    assert(transport.empty() == false);
    assert(transport.stream() == "8:ferienhaus-vdr");

    transport.clear();

    assert(transport.empty() == true);
}

int main()
{
    test_live_transport_interface_accepts_live_update_event();
    test_live_transport_interface_exposes_read_contract();

    std::cout
        << "test_live_transport_interface passed"
        << std::endl;

    return 0;
}
