#include "VdrOverviewService.h"

#include "VdrChannel.h"
#include "VdrService.h"
#include "VdrTimer.h"

#include <vector>

VdrOverviewService::VdrOverviewService(VdrService& vdrService)
    : vdrService_(vdrService)
{
}

VdrOverview VdrOverviewService::getOverview() const
{
    VdrOverview overview;

    overview.status = vdrService_.getStatus();

    std::vector<VdrChannel> channels =
        vdrService_.getChannels();

    overview.totalChannels =
        static_cast<int>(channels.size());

    for (const VdrChannel& channel : channels)
    {
        if (channel.radio)
        {
            ++overview.radioChannels;
        }

        if (channel.encrypted)
        {
            ++overview.encryptedChannels;
        }
    }

    overview.totalEvents =
        static_cast<int>(vdrService_.getEvents().size());

    std::vector<VdrTimer> timers =
        vdrService_.getTimers();

    overview.totalTimers =
        static_cast<int>(timers.size());

    for (const VdrTimer& timer : timers)
    {
        if (timer.enabled)
        {
            ++overview.activeTimers;
        }

        if (timer.recording)
        {
            ++overview.recordingTimers;
        }
    }

    overview.totalRecordings =
        static_cast<int>(vdrService_.getRecordings().size());

    return overview;
}
