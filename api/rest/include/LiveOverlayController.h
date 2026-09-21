#pragma once

#include "DashboardController.h"
#include "LiveOverlay.h"

#include <string>

class LiveOverlaySnapshotJsonSerializer
{
public:
    std::string serialize(const LiveOverlaySnapshot& snapshot) const;
};

class LiveOverlayController
{
public:
    LiveOverlayController(
        const LiveOverlayService& service,
        const LiveOverlaySnapshotJsonSerializer& serializer);

    ApiResponse getSnapshot(const std::string& backendId) const;

private:
    const LiveOverlayService& service_;
    const LiveOverlaySnapshotJsonSerializer& serializer_;
};
