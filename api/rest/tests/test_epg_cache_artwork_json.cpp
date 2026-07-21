#include "Database.h"
#include "EpgArtworkPublicJsonSerializer.h"
#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"
#include "EpgCacheController.h"
#include "EpgCacheService.h"
#include "EpgEventRepository.h"
#include "IVdrAdapter.h"
#include "VdrService.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

class ArtworkJsonEventAdapter : public IVdrAdapter
{
public:
    std::vector<VdrEvent> events;

    VdrStatus getStatus() const override { return VdrStatus{}; }
    std::vector<VdrEvent> getEvents() const override { return events; }
    std::vector<VdrEvent> getEvents(const VdrEventQuery&) const override { return events; }
    std::vector<VdrChannel> getChannels() const override { return {}; }
    std::vector<VdrTimer> getTimers() const override { return {}; }
    std::vector<VdrRecording> getRecordings() const override { return {}; }
    VdrChangeState getChangeState() const override { return VdrChangeState{}; }
};

static bool contains(const std::string& text, const std::string& value)
{
    return text.find(value) != std::string::npos;
}

static VdrEvent makeEvent(
    const std::string& id,
    const std::string& channelId,
    const std::string& title)
{
    VdrEvent event;
    event.id = id;
    event.channelId = channelId;
    event.title = title;
    event.startTime = "0900";
    event.endTime = "1000";
    event.durationSeconds = 3600;
    return event;
}

int main()
{
    const char* databasePath =
        "/tmp/vdr-suite-epg-cache-artwork-json-test.db";
    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));

    EpgEventRepository eventRepository(database);
    EpgArtworkRepository artworkRepository(database);
    assert(artworkRepository.ensureSchema());

    ArtworkJsonEventAdapter adapter;
    adapter.events = {
        makeEvent("event-1", "channel one/HD", "With Artwork"),
        makeEvent("event-2", "channel-2", "Without Artwork")};

    VdrService vdrService(adapter);
    EpgCacheService cacheService(eventRepository, vdrService);
    EpgArtworkPublicJsonSerializer artworkJsonSerializer;
    EpgCacheController controller(
        cacheService,
        artworkRepository,
        artworkJsonSerializer);

    VdrEventQuery query;
    query.limit = 2;

    const ApiResponse refresh =
        controller.refreshBackendWindow("home vdr", query);
    assert(refresh.statusCode == 200);

    EpgArtworkReference artwork;
    artwork.backendId = "home vdr";
    artwork.channelId = "channel one/HD";
    artwork.eventId = "event-1";
    artwork.provider = "tvscraper";
    artwork.path = "/var/cache/vdr/tvscraper/private-event-1.jpg";
    artwork.width = 1280;
    artwork.height = 720;
    artwork.resolvedAt = 123456;
    assert(artworkRepository.upsert(artwork));

    const ApiResponse response = controller.getWindow(
        "home vdr",
        "channel one/HD,channel-2",
        "0900",
        "1200",
        0);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(contains(response.body, "\"artwork\":{\"available\":true"));
    assert(contains(response.body, "\"provider\":\"tvscraper\""));
    assert(contains(response.body, "\"width\":1280"));
    assert(contains(response.body, "\"height\":720"));
    assert(contains(response.body, "backend=home%20vdr"));
    assert(contains(response.body, "channelId=channel%20one%2FHD"));
    assert(contains(response.body, "eventId=event-1"));
    assert(contains(response.body, "\"artwork\":{\"available\":false}"));
    assert(!contains(response.body, artwork.path));
    assert(!contains(response.body, "/var/cache"));

    return 0;
}
