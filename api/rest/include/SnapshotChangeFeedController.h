#pragma once

#include "DashboardController.h"

class SnapshotChangeFeed;
class SnapshotChangeFeedJsonSerializer;

class SnapshotChangeFeedController
{
public:
    SnapshotChangeFeedController(
        const SnapshotChangeFeed& feed,
        SnapshotChangeFeedJsonSerializer& serializer);

    ApiResponse getFeed();

private:
    const SnapshotChangeFeed& feed_;
    SnapshotChangeFeedJsonSerializer& serializer_;
};
