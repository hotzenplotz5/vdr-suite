#include "suitebridge.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_capability_discovery.h"
#include "suitebridge_command_result.h"
#include "suitebridge_epg_command_handler.h"
#include "suitebridge_plugin_identity.h"
#include "suitebridge_recording_metadata_command.h"
#include "suitebridge_svdrp_contract.h"

#include <vdr/tools.h>

namespace {

cString ReturnResult(
    const SuiteBridgeCommandResult &result,
    int &replyCode)
{
  replyCode = result.replyCode;
  return cString::sprintf("%s", result.payload.c_str());
}

} // namespace

const char **cPluginSuiteBridge::SVDRPHelpPages(void)
{
  static const char *HelpPages[] = {
      "CAPS [discovery-schema]\n"
      "    Return the read-only VDR-Suite capability discovery payload.",
      "SNAP\n"
      "    Return the current read-only VDR-Suite status payload.",
      "ARTW <channel-id> <event-id>\n"
      "    Resolve preferred TVScraper artwork for one EPG event.",
      "META <channel-id> <event-id>\n"
      "    Resolve bounded TVScraper metadata for one EPG event.",
      "MCOMPARE <channel-id> <event-id>\n"
      "    Compare Live-equivalent real-event TVScraper metadata with the detached META event snapshot.",
      "ETYPES <from-epoch> <until-epoch> <offset> <limit>\n"
      "    Return a bounded page of TVScraper movie/series types for real VDR EPG events.",
      "RMETA <recording-key>\n"
      "    Resolve bounded TVScraper metadata for one current VDR recording.",
      nullptr,
  };

  return HelpPages;
}

cString cPluginSuiteBridge::SVDRPCommand(
    const char *Command,
    const char *Option,
    int &ReplyCode)
{
  const SuiteBridgeCommandResult nativeProbe = nativeProbe_.Handle(
      Command,
      Option,
      [this]() {
        return statusMonitor_.CaptureSnapshot().MonitorActive();
      });
  if (nativeProbe.handled) return ReturnResult(nativeProbe, ReplyCode);

  const SuiteBridgeCommandResult liveSource = liveSource_.Handle(Command, Option);
  if (liveSource.handled) return ReturnResult(liveSource, ReplyCode);

  const SuiteBridgeCommandResult nativeTimerCreate =
      nativeTimerCreate_.Handle(Command, Option);
  if (nativeTimerCreate.handled) return ReturnResult(nativeTimerCreate, ReplyCode);

  const SuiteBridgeCommandResult nativeTimerDelete =
      nativeTimerDelete_.Handle(Command, Option);
  if (nativeTimerDelete.handled) return ReturnResult(nativeTimerDelete, ReplyCode);

  const SuiteBridgeCommandResult nativeTimerModify =
      nativeTimerModify_.Handle(Command, Option);
  if (nativeTimerModify.handled) return ReturnResult(nativeTimerModify, ReplyCode);

  const SuiteBridgeCapabilityDiscoveryReply capabilityReply(
      Command,
      Option,
      SuiteBridgePluginIdentity::Name,
      SuiteBridgePluginIdentity::Version);

  if (capabilityReply.Handled()) {
    ReplyCode = capabilityReply.ReplyCode();
    if (!capabilityReply.HasPayload()) {
      esyslog("suitebridge: svdrp command=CAPS result=rejected reply=%d", ReplyCode);
    } else {
      isyslog(
          "suitebridge: svdrp command=CAPS result=served reply=%d bytes=%zu schema=%u",
          ReplyCode,
          capabilityReply.Size(),
          SuiteBridgeCapabilityDiscoveryPayload::SchemaVersion());
    }
    return cString::sprintf("%s", capabilityReply.Data());
  }

  const SuiteBridgeCommandResult artwork =
      SuiteBridgeEpgCommandHandler::HandleArtwork(Command, Option);
  if (artwork.handled) return ReturnResult(artwork, ReplyCode);

  const SuiteBridgeCommandResult metadata =
      SuiteBridgeEpgCommandHandler::HandleMetadata(Command, Option);
  if (metadata.handled) return ReturnResult(metadata, ReplyCode);

  const SuiteBridgeCommandResult metadataComparison =
      SuiteBridgeEpgCommandHandler::HandleMetadataComparison(Command, Option);
  if (metadataComparison.handled) return ReturnResult(metadataComparison, ReplyCode);

  const SuiteBridgeCommandResult typeSnapshot =
      SuiteBridgeEpgCommandHandler::HandleTypeSnapshot(Command, Option);
  if (typeSnapshot.handled) return ReturnResult(typeSnapshot, ReplyCode);

  const SuiteBridgeCommandResult recordingMetadata =
      SuiteBridgeRecordingMetadataCommand::Handle(Command, Option);
  if (recordingMetadata.handled) return ReturnResult(recordingMetadata, ReplyCode);

  const SuiteBridgeSvdrpReply snapshotReply(
      Command,
      Option,
      SuiteBridgeCapabilities::SchemaVersion(),
      statusMonitor_.CaptureSnapshot());

  if (!snapshotReply.Handled()) return nullptr;

  ReplyCode = snapshotReply.ReplyCode();
  if (!snapshotReply.HasPayload()) {
    esyslog("suitebridge: svdrp command=SNAP result=rejected reply=%d", ReplyCode);
  } else {
    isyslog(
        "suitebridge: svdrp command=SNAP result=served reply=%d bytes=%zu",
        ReplyCode,
        snapshotReply.Size());
  }

  return cString::sprintf("%s", snapshotReply.Data());
}
