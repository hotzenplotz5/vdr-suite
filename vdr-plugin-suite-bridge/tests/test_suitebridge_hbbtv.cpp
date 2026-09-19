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
  int runtimeCalls = 0;
  int presentationCalls = 0;

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
    application.controlCode = 1;
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

  bool Runtime(
      VdrWebHbbtvRuntimeV1 &runtime,
      std::string &error) const override
  {
    if (!available) {
      error = "provider_unavailable";
      return false;
    }

    ++const_cast<FakeProvider *>(this)->runtimeCalls;
    runtime.schemaVersion = VDRWEB_HBBTV_RUNTIME_SCHEMA_V1;

    switch (runtime.operation) {
      case VDRWEB_HBBTV_RUNTIME_LAUNCH:
        runtime.result = VDRWEB_HBBTV_RUNTIME_RESULT_ACCEPTED;
        runtime.state = VDRWEB_HBBTV_RUNTIME_STATE_STARTING;
        return true;
      case VDRWEB_HBBTV_RUNTIME_STATUS:
        runtime.result = VDRWEB_HBBTV_RUNTIME_RESULT_OK;
        runtime.state = VDRWEB_HBBTV_RUNTIME_STATE_ACTIVE;
        return true;
      case VDRWEB_HBBTV_RUNTIME_INPUT:
        runtime.result = VDRWEB_HBBTV_RUNTIME_RESULT_OK;
        runtime.state = VDRWEB_HBBTV_RUNTIME_STATE_ACTIVE;
        return true;
      case VDRWEB_HBBTV_RUNTIME_CLOSE:
        runtime.result = VDRWEB_HBBTV_RUNTIME_RESULT_ACCEPTED;
        runtime.state = VDRWEB_HBBTV_RUNTIME_STATE_CLOSING;
        return true;
    }

    runtime.result = VDRWEB_HBBTV_RUNTIME_RESULT_INVALID_REQUEST;
    runtime.state = VDRWEB_HBBTV_RUNTIME_STATE_NONE;
    return true;
  }

  bool Presentation(
      VdrWebHbbtvPresentationV1 &presentation,
      std::string &error) const override
  {
    if (!available) {
      error = "provider_unavailable";
      return false;
    }

    ++const_cast<FakeProvider *>(this)->presentationCalls;
    presentation.schemaVersion = VDRWEB_HBBTV_PRESENTATION_SCHEMA_V1;
    presentation.result = VDRWEB_HBBTV_PRESENTATION_RESULT_OK;
    presentation.frameRevision = 9;
    presentation.observedAt = 1789671111;
    presentation.renderWidth = 1280;
    presentation.renderHeight = 720;
    presentation.encodedBytes = 4;

    if (presentation.operation == VDRWEB_HBBTV_PRESENTATION_CHUNK) {
      presentation.returnedBytes = 4;
      presentation.data[0] = 'q';
      presentation.data[1] = 'o';
      presentation.data[2] = 'i';
      presentation.data[3] = 'f';
    }
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
  static_assert(
      std::is_standard_layout<VdrWebHbbtvRuntimeV1>::value,
      "runtime ABI");
  static_assert(
      std::is_standard_layout<VdrWebHbbtvPresentationV1>::value,
      "presentation ABI");

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

  const SuiteBridgeCommandResult launch =
      service.Handle(
          "HBBRUN",
          "LAUNCH 1 session-a C-1-1051-10301 7 42");
  assert(launch.handled);
  assert(launch.replyCode == 250);
  assert(launch.payload.find(
      "\"capability\":\"broadcast.hbbtv.runtime\"") !=
      std::string::npos);
  assert(launch.payload.find("\"operation\":\"launch\"") !=
      std::string::npos);
  assert(launch.payload.find("\"result\":\"accepted\"") !=
      std::string::npos);
  assert(launch.payload.find("\"state\":\"starting\"") !=
      std::string::npos);
  assert(launch.payload.find("\"sessionId\":\"session-a\"") !=
      std::string::npos);
  assert(launch.payload.find("\"descriptorRevision\":42") !=
      std::string::npos);

  const SuiteBridgeCommandResult input =
      service.Handle(
          "HBBRUN",
          "INPUT 1 session-a C-1-1051-10301 7 42 LEFT");
  assert(input.handled);
  assert(input.replyCode == 250);
  assert(input.payload.find("\"action\":\"LEFT\"") !=
      std::string::npos);
  assert(input.payload.find("VK_LEFT") == std::string::npos);

  const SuiteBridgeCommandResult close =
      service.Handle(
          "HBBRUN",
          "CLOSE 1 session-a C-1-1051-10301 7 42");
  assert(close.handled);
  assert(close.replyCode == 250);
  assert(close.payload.find("\"state\":\"closing\"") !=
      std::string::npos);

  const SuiteBridgeCommandResult presentationMeta =
      service.Handle("HBBPRES", "META 1 session-a");
  assert(presentationMeta.handled);
  assert(presentationMeta.replyCode == 250);
  assert(presentationMeta.payload.find(
      "\"capability\":\"broadcast.hbbtv.presentation\"") !=
      std::string::npos);
  assert(presentationMeta.payload.find(
      "\"frameRevision\":9") != std::string::npos);
  assert(presentationMeta.payload.find(
      "\"renderWidth\":1280") != std::string::npos);
  assert(presentationMeta.payload.find("dataBase64") == std::string::npos);

  const SuiteBridgeCommandResult presentationChunk =
      service.Handle("HBBPRES", "CHUNK 1 session-a 9 0");
  assert(presentationChunk.handled);
  assert(presentationChunk.replyCode == 250);
  assert(presentationChunk.payload.find(
      "\"dataBase64\":\"cW9pZg==\"") != std::string::npos);
  assert(provider.presentationCalls == 2);

  const SuiteBridgeCommandResult invalidPresentation =
      service.Handle("HBBPRES", "CHUNK 1 session-a 0 0");
  assert(invalidPresentation.handled);
  assert(invalidPresentation.replyCode == 504);

  const SuiteBridgeCommandResult rawKey =
      service.Handle(
          "HBBRUN",
          "INPUT 1 session-a C-1-1051-10301 7 42 VK_LEFT");
  assert(rawKey.handled);
  assert(rawKey.replyCode == 504);

  const SuiteBridgeCommandResult invalidSession =
      service.Handle(
          "HBBRUN",
          "LAUNCH 1 bad/session C-1-1051-10301 7 42");
  assert(invalidSession.handled);
  assert(invalidSession.replyCode == 504);

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
      service.Handle(
          "HBBRUN",
          "STATUS 1 session-a C-1-1051-10301 7 42");
  assert(unavailable.handled);
  assert(unavailable.replyCode == 550);
  assert(unavailable.payload.find("provider_unavailable") != std::string::npos);

  return 0;
}
