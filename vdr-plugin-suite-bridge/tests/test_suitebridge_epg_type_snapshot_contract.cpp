#include "suitebridge_epg_type_snapshot_contract.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
  SuiteBridgeEpgTypeSnapshotRequest ignored("META", "1 2 0 64");
  assert(!ignored.Handled());

  SuiteBridgeEpgTypeSnapshotRequest invalidWindow(
      "ETYPES",
      "200 100 0 64");
  assert(invalidWindow.Handled());
  assert(!invalidWindow.Valid());

  SuiteBridgeEpgTypeSnapshotRequest invalidLimit(
      "ETYPES",
      "100 200 0 65");
  assert(invalidLimit.Handled());
  assert(!invalidLimit.Valid());

  SuiteBridgeEpgTypeSnapshotRequest request(
      "ETYPES",
      "100 200 64 32");
  assert(request.Handled());
  assert(request.Valid());
  assert(request.FromTime() == 100);
  assert(request.UntilTime() == 200);
  assert(request.Offset() == 64);
  assert(request.Limit() == 32);

  SuiteBridgeEpgTypeSnapshotPage page;
  page.nextOffset = 96;
  page.scanned = 32;
  page.done = false;

  SuiteBridgeEpgTypeSnapshotItem series;
  series.channelId = "S19.2E-1-1019-10301";
  series.eventId = 1234;
  series.startTime = 110;
  series.endTime = 170;
  series.mediaType = SuiteBridgeEpgMediaType::Series;
  page.items.push_back(series);

  SuiteBridgeEpgTypeSnapshotItem movie;
  movie.channelId = "C-1-1079-10351";
  movie.eventId = 5678;
  movie.startTime = 120;
  movie.endTime = 190;
  movie.mediaType = SuiteBridgeEpgMediaType::Movie;
  page.items.push_back(movie);

  SuiteBridgeEpgTypeSnapshotPayload payload(page);
  assert(payload.Complete());
  assert(std::strcmp(
      payload.Data(),
      "1|96|32|0|S19.2E-1-1019-10301,1234,110,170,S;"
      "C-1-1079-10351,5678,120,190,M") == 0);
  assert(payload.Size() == std::strlen(payload.Data()));

  SuiteBridgeEpgTypeSnapshotPage done;
  done.nextOffset = 12;
  done.scanned = 0;
  done.done = true;
  SuiteBridgeEpgTypeSnapshotPayload empty(done);
  assert(empty.Complete());
  assert(std::strcmp(empty.Data(), "1|12|0|1|") == 0);

  std::cout << "suitebridge epg type snapshot contract ok\n";
  return 0;
}
