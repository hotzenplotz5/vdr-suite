#include "suitebridge.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_capability_discovery.h"
#include "suitebridge_osd_snapshot_contract.h"
#include "suitebridge_plugin_identity.h"

#include <vdr/tools.h>

#include <sstream>
#include <vector>

cPluginSuiteBridge::cPluginSuiteBridge()
    : nativeProbe_(GenerateSuiteBridgePluginInstanceEpoch()),
      liveCapability_(nativeProbe_.PluginInstanceEpoch()),
      liveSource_(nativeProbe_.PluginInstanceEpoch()),
      hbbtvAdapter_(),
      hbbtvCommand_(&hbbtvAdapter_),
      teletextAdapter_(),
      teletextCommand_(&teletextAdapter_),
      nativeTimerCreate_(
          nativeProbe_.PluginInstanceEpoch(),
          &nativeTimerCreateVdrMutation_),
      nativeTimerDelete_(
          nativeProbe_.PluginInstanceEpoch(),
          &nativeTimerDeleteVdrMutation_),
      nativeTimerModify_(
          nativeProbe_.PluginInstanceEpoch(),
          &nativeTimerModifyVdrMutation_),
      recordingMarksModify_(
          nativeProbe_.PluginInstanceEpoch(),
          &recordingMarksModifyVdrMutation_),
      recordingCut_(
          nativeProbe_.PluginInstanceEpoch(),
          &recordingCutVdrMutation_)
{
}

cPluginSuiteBridge::~cPluginSuiteBridge() = default;

const char *cPluginSuiteBridge::Version(void)
{
  return SuiteBridgePluginIdentity::Version;
}

const char *cPluginSuiteBridge::Description(void)
{
  return SuiteBridgePluginIdentity::Description;
}

bool cPluginSuiteBridge::Initialize(void)
{
  if (!lifecycle_.Initialize()) {
    esyslog(
        "suitebridge: lifecycle event=initialize result=rejected state=%s",
        lifecycle_.StateName());
    return false;
  }

  isyslog(
      "suitebridge: lifecycle event=initialize result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      SuiteBridgePluginIdentity::Version);

  for (const auto &capability : SuiteBridgeCapabilities::All()) {
    isyslog(
        "suitebridge: capability schema=%u id=%s state=%s",
        SuiteBridgeCapabilities::SchemaVersion(),
        capability.id,
        SuiteBridgeCapabilities::StateName(capability.state));
  }

  isyslog(
      "suitebridge: native-operation=vdr.native.probe schema=1 side-effect=none mutations=disabled provider=suitebridge");
  isyslog(
      "suitebridge: native-operation=vdr.live.stream schema=1 transport=private-unix-stream receiver=bounded provider=suitebridge public-endpoint=none");
  isyslog(
      "suitebridge: native-read=legacy-osd.frame schema=1 provider=suitebridge transport=private-svdrp representation=semantic public-endpoint=none input=allowlisted");
  isyslog(
      "suitebridge: native-read=broadcast.teletext.page schema=1 provider=osdteletext transport=in-process-service public-endpoint=none");
  isyslog(
      "suitebridge: native-read=broadcast.hbbtv.discovery schema=1 provider=vdr-plugin-web transport=in-process-service public-endpoint=none");
  isyslog(
      "suitebridge: native-operation=broadcast.hbbtv.runtime schema=1 provider=vdr-plugin-web transport=in-process-service public-endpoint=none input=allowlisted");
  isyslog(
      "suitebridge: native-operation=vdr.timer.create schema=1 side-effect=timer-create mutations=enabled execution=enabled provider=suitebridge acceptance=required");
  isyslog(
      "suitebridge: native-operation=vdr.timer.delete schema=1 side-effect=timer-delete mutations=enabled execution=enabled provider=suitebridge acceptance=required");
  isyslog(
      "suitebridge: native-operation=vdr.timer.update schema=1 side-effect=timer-update mutations=enabled execution=enabled provider=suitebridge acceptance=required");
  isyslog(
      "suitebridge: native-operation=vdr.timer.toggle schema=1 side-effect=timer-toggle mutations=enabled execution=enabled provider=suitebridge acceptance=required");
  isyslog(
      "suitebridge: native-operation=vdr.recording.marks.modify schema=1 side-effect=recording-marks-modify mutations=enabled execution=enabled provider=suitebridge acceptance=required");
  isyslog(
      "suitebridge: native-operation=vdr.recording.cut schema=1 side-effect=recording-cut mutations=enabled execution=enabled provider=suitebridge acceptance=required");
  return true;
}

bool cPluginSuiteBridge::Start(void)
{
  if (!lifecycle_.Start()) {
    esyslog(
        "suitebridge: lifecycle event=start result=rejected state=%s",
        lifecycle_.StateName());
    return false;
  }

  statusMonitor_.Activate();

  if (!controlPlane_.Start(
          [this](
              SuiteBridgeControlPlane::Operation operation,
              const std::string &payload) {
            using Operation = vdrsuite::agent::control::Operation;
            auto split = [](const std::string &value) {
              std::vector<std::string> fields;
              std::size_t start = 0;
              while (true) {
                const std::size_t end = value.find('\n', start);
                fields.push_back(value.substr(
                    start,
                    end == std::string::npos
                        ? std::string::npos
                        : end - start));
                if (end == std::string::npos) break;
                start = end + 1;
              }
              return fields;
            };

            if (operation == Operation::LiveCapability) {
              if (!payload.empty()) {
                return SuiteBridgeCommandResult{
                    true, 501, "live_capability_payload_invalid"};
              }
              return liveCapability_.Handle("NLCAP", "1");
            }
            if (operation == Operation::NativeProbeCapability) {
              if (!payload.empty()) {
                return SuiteBridgeCommandResult{
                    true, 501, "native_probe_capability_payload_invalid"};
              }
              return nativeProbe_.Handle(
                  "NCAP",
                  "1",
                  [this]() {
                    return statusMonitor_.CaptureSnapshot().MonitorActive();
                  });
            }
            if (operation == Operation::CapabilityDiscovery) {
              if (!payload.empty()) {
                return SuiteBridgeCommandResult{
                    true, 501, "capability_discovery_payload_invalid"};
              }
              const SuiteBridgeCapabilityDiscoveryReply reply(
                  "CAPS",
                  "1",
                  SuiteBridgePluginIdentity::Name,
                  SuiteBridgePluginIdentity::Version);
              return SuiteBridgeCommandResult{
                  true,
                  reply.ReplyCode(),
                  reply.HasPayload()
                      ? std::string(reply.Data())
                      : std::string()};
            }
            if (operation == Operation::OsdSnapshot) {
              if (!payload.empty()) {
                return SuiteBridgeCommandResult{
                    true, 501, "osd_snapshot_payload_invalid"};
              }
              const SuiteBridgeOsdSnapshot snapshot =
                  statusMonitor_.CaptureOsdSnapshot();
              const SuiteBridgeOsdSnapshotPayload response(snapshot);
              if (!response.Complete()) {
                return SuiteBridgeCommandResult{
                    true,
                    451,
                    "Suite bridge OSD snapshot payload unavailable"};
              }
              return SuiteBridgeCommandResult{
                  true, 900, std::string(response.Data())};
            }

            if (operation == Operation::HbbtvDiscovery) {
              return hbbtvCommand_.Handle(
                  "HBBAPPS",
                  payload.c_str());
            }
            if (operation == Operation::HbbtvRuntime) {
              return hbbtvCommand_.Handle(
                  "HBBRUN",
                  payload.c_str());
            }
            if (operation == Operation::HbbtvPresentation) {
              return hbbtvCommand_.Handle(
                  "HBBPRES",
                  payload.c_str());
            }
            if (operation == Operation::HbbtvMedia) {
              return hbbtvCommand_.Handle(
                  "HBBMEDIA",
                  payload.c_str());
            }

            if (operation == Operation::TeletextCapability) {
              return teletextCommand_.Handle(
                  "TTXC",
                  payload.c_str());
            }
            if (operation == Operation::TeletextPage) {
              return teletextCommand_.Handle(
                  "TTXP",
                  payload.c_str());
            }

            const auto fields = split(payload);
            if (operation == Operation::OsdInput) {
              if (fields.size() != 10) {
                return SuiteBridgeCommandResult{
                    true, 501, "osd_input_payload_invalid"};
              }
              std::ostringstream option;
              option << "1 " << fields[0] << ' ' << fields[1] << ' '
                     << fields[2] << ' ' << fields[3] << ' ' << fields[4]
                     << ' ' << fields[5] << ' ' << fields[6] << ' '
                     << fields[7] << ' ' << fields[8] << ' ' << fields[9];
              return osdInput_.Handle(
                  "OSDINPUT",
                  option.str().c_str(),
                  statusMonitor_.CaptureOsdSnapshot());
            }
            if (operation == Operation::NativeProbeExecute) {
              if (fields.size() != 12) {
                return SuiteBridgeCommandResult{
                    true, 501, "native_probe_execute_payload_invalid"};
              }
              std::ostringstream option;
              option << "EXEC vdr-suite-native/1 vdr.native.probe 1 "
                     << fields[0] << ' ' << fields[1] << ' ' << fields[2] << ' '
                     << fields[3] << ' ' << fields[4] << ' ' << fields[5] << ' '
                     << fields[6] << ' ' << fields[7] << ' ' << fields[8] << ' '
                     << fields[9] << ' ' << fields[10] << " 1 " << fields[11];
              return nativeProbe_.Handle(
                  "NPROBE",
                  option.str().c_str(),
                  [this]() {
                    return statusMonitor_.CaptureSnapshot().MonitorActive();
                  });
            }
            if (operation == Operation::NativeProbeReadback) {
              if (fields.size() != 4) {
                return SuiteBridgeCommandResult{
                    true, 501, "native_probe_readback_payload_invalid"};
              }
              std::ostringstream option;
              option << "READ 1 " << fields[0] << ' ' << fields[1] << ' '
                     << fields[2] << ' ' << fields[3];
              return nativeProbe_.Handle(
                  "NPROBE",
                  option.str().c_str(),
                  [this]() {
                    return statusMonitor_.CaptureSnapshot().MonitorActive();
                  });
            }
            std::ostringstream option;
            if (operation == Operation::LiveOpen) {
              if (fields.size() != 3) {
                return SuiteBridgeCommandResult{
                    true, 501, "live_open_payload_invalid"};
              }
              option << "OPEN 1 " << fields[0] << ' ' << fields[1]
                     << ' ' << fields[2];
            } else if (
                operation == Operation::LiveStatus ||
                operation == Operation::LiveClose) {
              if (fields.size() != 2) {
                return SuiteBridgeCommandResult{
                    true, 501, "live_lease_payload_invalid"};
              }
              option << (operation == Operation::LiveStatus ? "STATUS" : "CLOSE")
                     << " 1 " << fields[0] << ' ' << fields[1];
            } else {
              return SuiteBridgeCommandResult{};
            }
            return liveSource_.Handle("NLIVE", option.str().c_str());
          },
          [](const std::string &message) {
            isyslog("%s", message.c_str());
          })) {
    statusMonitor_.Deactivate();
    (void)lifecycle_.BeginStop();
    (void)lifecycle_.CompleteStop();
    esyslog(
        "suitebridge: control-plane event=start result=rejected path=%s",
        controlPlane_.SocketPath().c_str());
    return false;
  }

  isyslog(
      "suitebridge: control-plane event=start result=accepted transport=af-unix-seqpacket path=%s",
      controlPlane_.SocketPath().c_str());

  isyslog(
      "suitebridge: lifecycle event=start result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      SuiteBridgePluginIdentity::Version);
  return true;
}

void cPluginSuiteBridge::Stop(void)
{
  if (!lifecycle_.BeginStop()) {
    esyslog(
        "suitebridge: lifecycle event=stop-begin result=rejected state=%s version=%s",
        lifecycle_.StateName(),
        SuiteBridgePluginIdentity::Version);
    return;
  }

  if (lifecycle_.State() == SuiteBridgeLifecycleState::Stopping) {
    isyslog(
        "suitebridge: lifecycle event=stop-begin result=accepted state=%s version=%s",
        lifecycle_.StateName(),
        SuiteBridgePluginIdentity::Version);

    controlPlane_.Stop();
    const auto controlMetrics = controlPlane_.SnapshotMetrics();
    isyslog(
        "suitebridge: control-plane event=stop admitted=%llu executed=%llu rejected=%llu overloaded=%llu deadline-expired=%llu critical-high-water=%llu interactive-high-water=%llu external-plugin-high-water=%llu",
        static_cast<unsigned long long>(
            controlMetrics.admittedByOperation[0] +
            controlMetrics.admittedByOperation[1] +
            controlMetrics.admittedByOperation[2] +
            controlMetrics.admittedByOperation[3] +
            controlMetrics.admittedByOperation[4] +
            controlMetrics.admittedByOperation[5] +
            controlMetrics.admittedByOperation[6] +
            controlMetrics.admittedByOperation[7] +
            controlMetrics.admittedByOperation[8] +
            controlMetrics.admittedByOperation[9] +
            controlMetrics.admittedByOperation[10] +
            controlMetrics.admittedByOperation[11] +
            controlMetrics.admittedByOperation[12] +
            controlMetrics.admittedByOperation[13] +
            controlMetrics.admittedByOperation[14] +
            controlMetrics.admittedByOperation[15]),
        static_cast<unsigned long long>(
            controlMetrics.executedByOperation[0] +
            controlMetrics.executedByOperation[1] +
            controlMetrics.executedByOperation[2] +
            controlMetrics.executedByOperation[3] +
            controlMetrics.executedByOperation[4] +
            controlMetrics.executedByOperation[5] +
            controlMetrics.executedByOperation[6] +
            controlMetrics.executedByOperation[7] +
            controlMetrics.executedByOperation[8] +
            controlMetrics.executedByOperation[9] +
            controlMetrics.executedByOperation[10] +
            controlMetrics.executedByOperation[11] +
            controlMetrics.executedByOperation[12] +
            controlMetrics.executedByOperation[13] +
            controlMetrics.executedByOperation[14] +
            controlMetrics.executedByOperation[15]),
        static_cast<unsigned long long>(controlMetrics.rejected),
        static_cast<unsigned long long>(controlMetrics.overloaded),
        static_cast<unsigned long long>(
            controlMetrics.deadlineExpiredBeforeExecution),
        static_cast<unsigned long long>(
            controlMetrics.queueHighWaterByClass[0]),
        static_cast<unsigned long long>(
            controlMetrics.queueHighWaterByClass[1]),
        static_cast<unsigned long long>(
            controlMetrics.queueHighWaterByClass[2]));
    liveSource_.StopAll();
    statusMonitor_.Deactivate();

    if (!lifecycle_.CompleteStop()) {
      esyslog(
          "suitebridge: lifecycle event=stop-complete result=rejected state=%s version=%s",
          lifecycle_.StateName(),
          SuiteBridgePluginIdentity::Version);
      return;
    }
  }

  isyslog(
      "suitebridge: lifecycle event=stop-complete result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      SuiteBridgePluginIdentity::Version);
}

void cPluginSuiteBridge::MainThreadHook(void)
{
  statusMonitor_.ObserveRecordingListState();
}

const char *cPluginSuiteBridge::MainMenuEntry(void)
{
  return nullptr;
}

VDRPLUGINCREATOR(cPluginSuiteBridge);
