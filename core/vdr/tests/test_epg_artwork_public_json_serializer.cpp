#include "EpgArtworkPublicJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{
bool contains(
    const std::string& text,
    const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}
}

int main()
{
    EpgArtworkPublicJsonSerializer serializer;

    EpgArtworkReference artwork;
    artwork.backendId = "remote vdr";
    artwork.channelId = "S19.2E-1-1019-10301/HD";
    artwork.eventId = "47&11";
    artwork.provider = "tv\"scraper";
    artwork.path = "/var/cache/vdr/plugins/tvscraper/private/still.jpg";
    artwork.width = 640;
    artwork.height = 360;
    artwork.resolvedAt = 123456;

    const std::string json = serializer.serialize(artwork);

    assert(contains(json, "\"available\":true"));
    assert(contains(json, "\"provider\":\"tv\\\"scraper\""));
    assert(contains(json, "\"width\":640"));
    assert(contains(json, "\"height\":360"));
    assert(contains(
        json,
        "/api/epg/cache/artwork?backend=remote%20vdr&channelId=S19.2E-1-1019-10301%2FHD&eventId=47%2611"));
    assert(!contains(json, artwork.path));
    assert(!contains(json, "/var/cache"));

    EpgArtworkReference missing;
    assert(serializer.serialize(missing) == "{\"available\":false}");
    assert(serializer.artworkUrl(missing).empty());

    std::cout << "test_epg_artwork_public_json_serializer passed" << std::endl;
    return 0;
}
