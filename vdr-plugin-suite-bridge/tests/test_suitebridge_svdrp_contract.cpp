#include "suitebridge_local_contract.h"
#include "suitebridge_status_snapshot.h"
#include "suitebridge_svdrp_contract.h"

#include <cassert>
#include <cstring>
#include <type_traits>

int main()
{
  static_assert(
      std::is_copy_constructible<SuiteBridgeSvdrpReply>::value);
  static_assert(
      !std::is_copy_assignable<SuiteBridgeSvdrpReply>::value);
  static_assert(SuiteBridgeSvdrpReply::SuccessReplyCode() == 900);
  static_assert(SuiteBridgeSvdrpReply::InvalidOptionReplyCode() == 504);
  static_assert(SuiteBridgeSvdrpReply::PayloadUnavailableReplyCode() == 451);
  static_assert(SuiteBridgeLocalContractPayload::SchemaVersion() == 2);

  const SuiteBridgeStatusSnapshot snapshot(
      true,
      12,
      2,
      3,
      4,
      "0123456789abcdef0123456789abcdef",
      false);

  const SuiteBridgeSvdrpReply reply(
      "SNAP",
      "",
      1,
      snapshot);
  const char *expected =
      "{\"contract_schema\":2,\"capability_schema\":1,\"snapshot_schema\":2,\"active\":true,\"total\":21,\"channel_switch\":12,\"recording\":2,\"replaying\":3,\"timer_change\":4,\"counter_epoch\":\"0123456789abcdef0123456789abcdef\",\"counter_overflow\":false}";

  assert(reply.Handled());
  assert(reply.HasPayload());
  assert(reply.ReplyCode() == 900);
  assert(reply.Size() == std::strlen(expected));
  assert(std::strcmp(reply.Data(), expected) == 0);

  const SuiteBridgeSvdrpReply lowerCase(
      "snap",
      "",
      1,
      snapshot);

  assert(lowerCase.Handled());
  assert(lowerCase.HasPayload());
  assert(lowerCase.ReplyCode() == 900);
  assert(std::strcmp(lowerCase.Data(), expected) == 0);

  const SuiteBridgeSvdrpReply invalidOption(
      "SNAP",
      "NOW",
      1,
      snapshot);

  assert(invalidOption.Handled());
  assert(!invalidOption.HasPayload());
  assert(invalidOption.ReplyCode() == 504);
  assert(std::strcmp(
      invalidOption.Data(),
      "SNAP does not accept options") == 0);
  assert(invalidOption.Size() ==
      std::strlen("SNAP does not accept options"));

  const SuiteBridgeSvdrpReply unknown(
      "UNKNOWN",
      "",
      1,
      snapshot);

  assert(!unknown.Handled());
  assert(!unknown.HasPayload());
  assert(unknown.ReplyCode() == 0);
  assert(unknown.Data() == nullptr);
  assert(unknown.Size() == 0);

  const SuiteBridgeSvdrpReply nullCommand(
      nullptr,
      nullptr,
      1,
      snapshot);

  assert(!nullCommand.Handled());
  assert(nullCommand.Data() == nullptr);

  return 0;
}
