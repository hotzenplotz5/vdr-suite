#include "../suitebridge_teletext_command.h"

#include <cassert>
#include <cstring>
#include <string>
#include <type_traits>

namespace {

class FakeProvider final : public ISuiteBridgeTeletextProvider {
public:
  bool available = true;

  bool ReadCapabilities(
      OsdTeletextCapabilitiesV1 &capabilities,
      std::string &error) const override
  {
    if (!available) {
      error = "provider_unavailable";
      return false;
    }
    capabilities = {};
    capabilities.structSize = sizeof(capabilities);
    capabilities.schemaVersion = OSDTELETEXT_SERVICE_SCHEMA_V1;
    capabilities.rows = OSDTELETEXT_PAGE_ROWS;
    capabilities.columns = OSDTELETEXT_PAGE_COLUMNS;
    capabilities.maxSnapshots = 2048;
    capabilities.pageRead = 1;
    capabilities.subpages = 1;
    capabilities.level1 = 1;
    capabilities.x26Partial = 1;
    capabilities.conceal = 1;
    capabilities.blink = 1;
    capabilities.doubleSize = 1;
    return true;
  }

  bool ReadPage(
      const std::string &channelId,
      std::uint16_t pageNumber,
      std::uint16_t,
      OsdTeletextGetPageV1 &page,
      std::string &error) const override
  {
    if (!available) {
      error = "provider_unavailable";
      return false;
    }
    page = {};
    page.structSize = sizeof(page);
    page.schemaVersion = OSDTELETEXT_SERVICE_SCHEMA_V1;
    page.result = channelId == "C-1-1051-10301"
        ? OSDTELETEXT_RESULT_OK
        : OSDTELETEXT_RESULT_CHANNEL_MISMATCH;
    page.sourceState = OSDTELETEXT_SOURCE_LIVE;
    page.receiverActive = 1;
    page.teletextAvailable = 1;
    page.serviceEpoch = 7;
    page.pageRevision = 42;
    page.observedAt = 1789668886;
    page.resolvedPageNumber = pageNumber;
    page.resolvedSubpageCode = 1;
    page.rows = OSDTELETEXT_PAGE_ROWS;
    page.columns = OSDTELETEXT_PAGE_COLUMNS;
    for (auto &cell : page.cells) {
      cell.codepoint = ' ';
      cell.kind = OSDTELETEXT_CELL_TEXT;
    }
    const char *text = "ARD TEXT";
    for (std::size_t index = 0; text[index] != '\0'; ++index) {
      page.cells[index].codepoint = static_cast<unsigned char>(text[index]);
    }
    return true;
  }
};

} // namespace

int main()
{
  static_assert(std::is_standard_layout<OsdTeletextCapabilitiesV1>::value, "capabilities ABI");
  static_assert(std::is_standard_layout<OsdTeletextCellV1>::value, "cell ABI");
  static_assert(std::is_standard_layout<OsdTeletextGetPageV1>::value, "page ABI");
  static_assert(OSDTELETEXT_PAGE_CELL_COUNT == 1000, "40x25 page shape");

  FakeProvider provider;
  SuiteBridgeTeletextCommandService service(&provider);

  const SuiteBridgeCommandResult capabilities = service.Handle("TTXC", "1");
  assert(capabilities.handled);
  assert(capabilities.replyCode == 250);
  assert(capabilities.payload.find("\"capability\":\"broadcast.teletext.page\"") != std::string::npos);
  assert(capabilities.payload.find("\"available\":true") != std::string::npos);

  const SuiteBridgeCommandResult page =
      service.Handle("TTXP", "1 C-1-1051-10301 100 auto");
  assert(page.handled);
  assert(page.replyCode == 250);
  assert(page.payload.find("\"result\":\"ok\"") != std::string::npos);
  assert(page.payload.find("\"page\":100") != std::string::npos);
  assert(page.payload.find("\"receiverActive\":true") != std::string::npos);
  assert(page.payload.find("\"text\":[\"ARD TEXT") != std::string::npos);
  assert(page.payload.find("\"cells\":[[") != std::string::npos);

  const SuiteBridgeCommandResult mismatch =
      service.Handle("TTXP", "1 C-1-1-1 100 auto");
  assert(mismatch.handled);
  assert(mismatch.replyCode == 250);
  assert(mismatch.payload.find("\"result\":\"channel_mismatch\"") != std::string::npos);

  const SuiteBridgeCommandResult invalid =
      service.Handle("TTXP", "2 C-1-1051-10301 100 auto");
  assert(invalid.handled);
  assert(invalid.replyCode == 504);

  provider.available = false;
  const SuiteBridgeCommandResult unavailable = service.Handle("TTXC", "1");
  assert(unavailable.handled);
  assert(unavailable.replyCode == 550);
  assert(unavailable.payload.find("provider_unavailable") != std::string::npos);

  return 0;
}
