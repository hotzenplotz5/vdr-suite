#include "SnapshotChangeFeedController.h"

#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedJsonSerializer.h"

SnapshotChangeFeedController::SnapshotChangeFeedController(
    const SnapshotChangeFeed& feed,
    SnapshotChangeFeedJsonSerializer& serializer)
    : feed_(feed),
      serializer_(serializer)
{
}

ApiResponse SnapshotChangeFeedController::getFeed()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        serializer_.serializeFeed(feed_);

    return response;
}
