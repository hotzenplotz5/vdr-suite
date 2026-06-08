#ifndef SNAPSHOT_CHANGE_FEED_JSON_SERIALIZER_H
#define SNAPSHOT_CHANGE_FEED_JSON_SERIALIZER_H

#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedEntry.h"

#include <string>

class SnapshotChangeFeedJsonSerializer {
public:
    std::string serializeEntry(
        const SnapshotChangeFeedEntry& entry) const;

    std::string serializeFeed(
        const SnapshotChangeFeed& feed) const;
};

#endif
