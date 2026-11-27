#ifndef SSE_LIVE_TRANSPORT_H
#define SSE_LIVE_TRANSPORT_H

#include "ILiveTransport.h"
#include "LiveUpdateEvent.h"
#include "LiveUpdateEventJsonSerializer.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class DeviceLiveEventHub;

class SseLiveTransport : public ILiveTransport {
public:
    static constexpr std::size_t maxRetainedFrames = 256;
    static constexpr std::size_t maxRetainedBytes = 512 * 1024;

    SseLiveTransport();

    void setDeviceLiveEventHub(std::shared_ptr<DeviceLiveEventHub> hub) {
        deviceLiveEventHub_ = std::move(hub);
    }
    // Browser SSE may project already-bound opaque Recording IDs from the
    // canonical identity repository. Device events remain unchanged.
    using RecordingPublicIdLookup = std::function<std::optional<std::string>(
        const std::string&, const std::string&)>;
    void setRecordingPublicIdLookup(RecordingPublicIdLookup lookup) {
        recordingPublicIdLookup_ = std::move(lookup);
    }
    void publish(const LiveUpdateEvent& event) override;

    const std::vector<std::string>& frames() const;
    std::string stream() const override;
    std::string replay(const std::string& lastEventId) const override;
    std::shared_ptr<BrowserLiveEventSubscription> subscribeBrowser(
        const std::string& lastEventId) const override;
    bool empty() const override;
    void clear() override;

    const std::string& daemonEpoch() const { return daemonEpoch_; }
    int oldestSequenceNumber() const;
    int latestSequenceNumber() const { return latestSequenceNumber_; }

private:
    std::string resyncFrame(const char* reason) const;
    void notifyBrowserSubscribers(
        const std::string& frame, bool gap);
    mutable std::vector<std::weak_ptr<BrowserLiveEventSubscription>>
        browserSubscribers_;

    std::vector<std::string> frames_;
    std::vector<int> frameSequenceNumbers_;
    std::size_t retainedBytes_ = 0;
    int latestSequenceNumber_ = 0;
    std::string daemonEpoch_;
    LiveUpdateEventJsonSerializer serializer_;
    RecordingPublicIdLookup recordingPublicIdLookup_;
    std::shared_ptr<DeviceLiveEventHub> deviceLiveEventHub_;
};

#endif
