#include "../suitebridge_hbbtv_command.h"

#include <cassert>
#include <cstring>
#include <string>
#include <type_traits>

namespace {

void copyText(char *target, std::size_t capacity, const char *value)
{
  assert(target != nullptr);
  assert(value != nullptr);
  const std::size_t length = std::strlen(value);
  assert(length < capacity);
  std::memcpy(target, value, length + 1U);
}

class FakeProvider final : public ISuiteBridgeHbbtvProvider {
public:
  bool available = true;

  bool Discover(
      const std::string &channelId,
      VdrWebHbbtvDiscoveryV1 &discovery,
      std::string &error) const override
  {
    if (!available) {
      error = "provider_unavailable";
      return false;
    }

    discovery = {};
    discovery.structSize = sizeof(discovery);
    discovery.schemaVersion = VDRWEB_HBBTV_SERVICE_SCHEMA_V1;
    discovery.receiverActive = 1;
    discovery.discoveryRevision = 42;
    discovery.observedAt = 1789668886;

    if (channelId != "C-1-1051-10301") {
      discovery.result = VDRWEB_HBBTV_RESULT_CHANNEL_MISMATCH;
      return true;
    }

    discovery.result = VDRWEB_HBBTV_RESULT_OK;
    discovery.applicationCount = 1;
    auto &application = discovery.applications[0];
    application.applicationId = 7;
    application.controlCode = 2;
    application.priority = 5;
    copyText(
        application.name,
        VDRWEB_HBBTV_APPLICATION_NAME_MAX,
        "ARD HbbTV");
    copyText(
        application.urlBase,
        VDRWEB_HBBTV_URL_BASE_MAX,
        "https://example.invalid/");
    copyText(
        application.urlLocation,
        VDRWEB_HBBTV_URL_LOCATION_MAX,
        "index.html");
    return true;
  }
};

} // namespace

int main()
{
  static_assert(
      std::is_standard_layout<VdrWebHbbtvApplicationV1>::value,
      "application ABI");
  static_assert(
      std::is_standard_layout<VdrWebHbbtvDiscoveryV1>::value,
      "discovery ABI");

  FakeProvider provider;
  SuiteBridgeHbbtvCommandService service(&provider);

  const SuiteBridgeCommandResult discovery =
      service.Handle("HBBAPPS", "1 C-1-1051-10301");
  assert(discovery.handled);
  assert(discovery.replyCode == 250);
  assert(discovery.payload.find(
      "\"capability\":\"broadcast.hbbtv.discovery\"") !=
      std::string::npos);
  assert(discovery.payload.find(
      "\"provider\":\"vdr-plugin-web\"") != std::string::npos);
  assert(discovery.payload.find("\"result\":\"ok\"") != std::string::npos);
  assert(discovery.payload.find("\"applicationId\":7") != std::string::npos);
  assert(discovery.payload.find("\"name\":\"ARD HbbTV\"") != std::string::npos);
  assert(discovery.payload.find(
      "\"urlBase\":\"https://example.invalid/\"") != std::string::npos);

  const SuiteBridgeCommandResult mismatch =
      service.Handle("HBBAPPS", "1 C-1-1-1");
  assert(mismatch.handled);
  assert(mismatch.replyCode == 250);
  assert(mismatch.payload.find(
      "\"result\":\"channel_mismatch\"") != std::string::npos);

  const SuiteBridgeCommandResult invalid =
      service.Handle("HBBAPPS", "2 C-1-1051-10301");
  assert(invalid.handled);
  assert(invalid.replyCode == 504);

  provider.available = false;
  const SuiteBridgeCommandResult unavailable =
      service.Handle("HBBAPPS", "1 C-1-1051-10301");
  assert(unavailable.handled);
  assert(unavailable.replyCode == 550);
  assert(unavailable.payload.find("provider_unavailable") != std::string::npos);

  return 0;
}
