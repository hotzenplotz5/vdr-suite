#include "suitebridge_epg_artwork_contract.h"

#include <cassert>
#include <string>

int main()
{
  {
    SuiteBridgeEpgArtworkRequest request("ARTW", "S19.2E-1-1019-10301 12345");
    assert(request.Handled());
    assert(request.Valid());
    assert(request.ChannelId() == "S19.2E-1-1019-10301");
    assert(request.EventId() == 12345U);
  }

  {
    SuiteBridgeEpgArtworkRequest request("artw", " channel 42 ");
    assert(request.Handled());
    assert(request.Valid());
    assert(request.ChannelId() == "channel");
    assert(request.EventId() == 42U);
  }

  {
    SuiteBridgeEpgArtworkRequest request("ARTW", "channel");
    assert(request.Handled());
    assert(!request.Valid());
  }

  {
    SuiteBridgeEpgArtworkRequest request("SNAP", "channel 42");
    assert(!request.Handled());
    assert(!request.Valid());
  }

  {
    SuiteBridgeArtworkReference artwork;
    artwork.provider = SuiteBridgeArtworkProvider::TvScraper;
    artwork.path = "/var/cache/tvscraper/a\"b.jpg";
    artwork.width = 1280;
    artwork.height = 720;

    SuiteBridgeEpgArtworkPayload payload(artwork);
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());
    assert(json.find("\"found\":true") != std::string::npos);
    assert(json.find("\"provider\":\"tvscraper\"") != std::string::npos);
    assert(json.find("a\\\"b.jpg") != std::string::npos);
    assert(json.find("\"width\":1280") != std::string::npos);
    assert(json.find("\"height\":720") != std::string::npos);
  }

  {
    SuiteBridgeEpgArtworkPayload payload(SuiteBridgeArtworkReference{});
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());
    assert(json.find("\"found\":false") != std::string::npos);
    assert(json.find("\"provider\":\"none\"") != std::string::npos);
  }

  return 0;
}
