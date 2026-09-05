#include "suitebridge_capabilities.h"
#include "suitebridge_capability_discovery.h"
#include "suitebridge_local_contract.h"
#include "suitebridge_status_snapshot.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <type_traits>

int main()
{
  static_assert(
      std::is_copy_constructible<
          SuiteBridgeCapabilityDiscoveryPayload>::value);
  static_assert(
      !std::is_copy_assignable<
          SuiteBridgeCapabilityDiscoveryPayload>::value);
  static_assert(
      std::is_copy_constructible<
          SuiteBridgeCapabilityDiscoveryReply>::value);
  static_assert(
      !std::is_copy_assignable<
          SuiteBridgeCapabilityDiscoveryReply>::value);

  static_assert(SuiteBridgeCapabilityDiscoveryPayload::SchemaVersion() == 1);
  static_assert(SuiteBridgeCapabilities::SchemaVersion() == 1);
  static_assert(SuiteBridgeStatusSnapshot::SchemaVersion() == 2);
  static_assert(SuiteBridgeLocalContractPayload::SchemaVersion() == 2);
  static_assert(SuiteBridgeCapabilityDiscoveryReply::SuccessReplyCode() == 900);
  static_assert(
      SuiteBridgeCapabilityDiscoveryReply::MalformedOptionReplyCode() == 501);
  static_assert(
      SuiteBridgeCapabilityDiscoveryReply::UnsupportedSchemaReplyCode() == 504);
  static_assert(
      SuiteBridgeCapabilityDiscoveryReply::PayloadUnavailableReplyCode() == 451);

  const char *expected =
      "{\"discovery_schema\":1,\"plugin_name\":\"suitebridge\",\"plugin_version\":\"0.13.0\",\"capability_schema\":1,\"snapshot_schema\":2,\"local_contract_schema\":2,\"capabilities\":[{\"id\":\"lifecycle\",\"state\":\"available\"},{\"id\":\"status-events\",\"state\":\"available\"},{\"id\":\"snapshots\",\"state\":\"available\"},{\"id\":\"local-contract\",\"state\":\"available\"},{\"id\":\"recording-metadata\",\"state\":\"available\"},{\"id\":\"recording-marks\",\"state\":\"available\"},{\"id\":\"epg-type-snapshot\",\"state\":\"available\"},{\"id\":\"vdr.live.stream\",\"state\":\"available\"},{\"id\":\"mutations\",\"state\":\"disabled\"}]}";

  const SuiteBridgeCapabilityDiscoveryPayload payload(
      "suitebridge",
      "0.13.0");

  assert(payload.Complete());
  assert(payload.Size() == std::strlen(expected));
  assert(std::strcmp(payload.Data(), expected) == 0);

  const SuiteBridgeCapabilityDiscoveryReply current(
      "CAPS",
      "",
      "suitebridge",
      "0.13.0");
  const SuiteBridgeCapabilityDiscoveryReply explicitCurrent(
      "CAPS",
      "1",
      "suitebridge",
      "0.13.0");
  const SuiteBridgeCapabilityDiscoveryReply paddedCurrent(
      "caps",
      " 01 ",
      "suitebridge",
      "0.13.0");

  for (const auto *reply : {&current, &explicitCurrent, &paddedCurrent}) {
    assert(reply->Handled());
    assert(reply->HasPayload());
    assert(reply->ReplyCode() == 900);
    assert(reply->Size() == std::strlen(expected));
    assert(std::strcmp(reply->Data(), expected) == 0);
  }

  const SuiteBridgeCapabilityDiscoveryReply unsupported(
      "CAPS",
      "2",
      "suitebridge",
      "0.13.0");
  const SuiteBridgeCapabilityDiscoveryReply oversized(
      "CAPS",
      "999999999999999999999999999999999999999999",
      "suitebridge",
      "0.13.0");

  for (const auto *reply : {&unsupported, &oversized}) {
    assert(reply->Handled());
    assert(!reply->HasPayload());
    assert(reply->ReplyCode() == 504);
    assert(std::strcmp(
        reply->Data(),
        "CAPS discovery schema unsupported") == 0);
  }

  const SuiteBridgeCapabilityDiscoveryReply malformedText(
      "CAPS",
      "abc",
      "suitebridge",
      "0.13.0");
  const SuiteBridgeCapabilityDiscoveryReply malformedSuffix(
      "CAPS",
      "1 extra",
      "suitebridge",
      "0.13.0");

  for (const auto *reply : {&malformedText, &malformedSuffix}) {
    assert(reply->Handled());
    assert(!reply->HasPayload());
    assert(reply->ReplyCode() == 501);
    assert(std::strcmp(
        reply->Data(),
        "CAPS requires a decimal discovery schema") == 0);
  }

  std::array<char, 1024> oversizedVersion{};
  std::fill(oversizedVersion.begin(), oversizedVersion.end() - 1, 'x');

  const SuiteBridgeCapabilityDiscoveryReply unavailable(
      "CAPS",
      "1",
      "suitebridge",
      oversizedVersion.data());

  assert(unavailable.Handled());
  assert(!unavailable.HasPayload());
  assert(unavailable.ReplyCode() == 451);
  assert(std::strcmp(
      unavailable.Data(),
      "Suite bridge capability payload unavailable") == 0);

  const SuiteBridgeCapabilityDiscoveryReply unknown(
      "UNKNOWN",
      "",
      "suitebridge",
      "0.13.0");
  const SuiteBridgeCapabilityDiscoveryReply nullCommand(
      nullptr,
      nullptr,
      "suitebridge",
      "0.13.0");

  for (const auto *reply : {&unknown, &nullCommand}) {
    assert(!reply->Handled());
    assert(!reply->HasPayload());
    assert(reply->ReplyCode() == 0);
    assert(reply->Data() == nullptr);
    assert(reply->Size() == 0);
  }

  return 0;
}
