#include "VdrOverviewService.h"

#include "ISnapshotAccessService.h"
#include "VdrChannel.h"
#include "VdrRecording.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrTimer.h"

#include <vector>

VdrOverviewService::VdrOverviewService(VdrService& vdrService)
    : vdrService_(&vdrService)
{
}

VdrOverviewService::VdrOverviewService(
    ISnapshotAccessService& snapshotAccessService)
    : snapshotAccessService_(&snapshotAccessService)
{
}

VdrOverview VdrOverviewService::getOverview() const
{
    if (snapshotAccessService_ != nullptr)
    {
        return getSnapshotOverview();
    }

    return getLiveOverview();
}

VdrOverview VdrOverviewService::getLiveOverview() const
{
    VdrOverview overview;

    overview.status = vdrService_->getStatus();

    std::vector<VdrChannel> channels = vdrService_->getChannels();
    overview.totalChannels = static_cast<int>(channels.size());

    for (const VdrChannel& channel : channels)
    {
        if (channel.radio) ++overview.radioChannels;
        if (channel.encrypted) ++overview.encryptedChannels;
    }

    overview.totalEvents = static_cast<int>(vdrService_->getEvents().size());

    std::vector<VdrTimer> timers = vdrService_->getTimers();
    overview.totalTimers = static_cast<int>(timers.size());

    for (const VdrTimer& timer : timers)
    {
        if (timer.enabled)
        {
            ++overview.activeTimers;
            if (!overview.hasNextTimer)
            {
                overview.nextTimer = timer;
                overview.hasNextTimer = true;
            }
        }
        if (timer.recording) ++overview.recordingTimers;
    }

    std::vector<VdrRecording> recordings = vdrService_->getRecordings();
    overview.totalRecordings = static_cast<int>(recordings.size());

    if (!recordings.empty())
    {
        overview.latestRecording = recordings.front();
        overview.hasLatestRecording = true;
    }

    return overview;
}

VdrOverview VdrOverviewService::getSnapshotOverview() const
{
    VdrOverview overview;

    if (!snapshotAccessService_->hasSnapshot())
    {
        return overview;
    }

    const VdrSnapshot* snapshot = snapshotAccessService_->snapshot();

    overview.status = snapshot->status;
    overview.totalChannels = static_cast<int>(snapshot->channels.size());
    overview.totalEvents = static_cast<int>(snapshot->events.size());
    overview.totalTimers = static_cast<int>(snapshot->timers.size());
    overview.totalRecordings = static_cast<int>(snapshot->recordings.size());

    for (const auto& channel : snapshot->channels)
    {
        if (channel.radio) ++overview.radioChannels;
        if (channel.encrypted) ++overview.encryptedChannels;
    }

    for (const auto& timer : snapshot->timers)
    {
        if (timer.enabled)
        {
            ++overview.activeTimers;
            if (!overview.hasNextTimer)
            {
                overview.nextTimer = timer;
                overview.hasNextTimer = true;
            }
        }

        if (timer.recording) ++overview.recordingTimers;
    }

    if (!snapshot->recordings.empty())
    {
        overview.latestRecording = snapshot->recordings.front();
        overview.hasLatestRecording = true;
    }

    return overview;
}
