#include "suitebridge_native_timer_preview_service.h"

#include "BackendAgentNativeTimerDeleteFingerprint.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <limits>
#include <sstream>
#include <string>
#include <strings.h>
#include <utility>
#include <vector>

namespace {
using namespace vdrsuite_native_timer_preview;

SuiteBridgeCommandResult reply(int status, const std::string& payload)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = status;
  result.payload = payload;
  return result;
}

std::vector<std::string> words(const char* option)
{
  if (!option) return {};
  std::string line(option);
  if (line.size() > 5500) return {};
  std::istringstream input(line);
  std::vector<std::string> parts;
  for (std::string word; input >> word;) {
    parts.push_back(word);
    if (parts.size() > 17) return {};
  }
  return parts;
}

bool decodeHex(const std::string& source, std::string& output,
               std::size_t limit)
{
  output.clear();
  if (source == "-") return true;
  if (source.empty() || source.size() > limit * 2 ||
      source.size() % 2 != 0) return false;
  for (std::size_t i = 0; i < source.size(); i += 2) {
    auto nibble = [](unsigned char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return 10 + c - 'a';
      return -1;
    };
    int a = nibble(source[i]), b = nibble(source[i + 1]);
    if (a < 0 || b < 0) { output.clear(); return false; }
    char c = static_cast<char>((a << 4) | b);
    if (c == 0 || c == '\r' || c == '\n') {
      output.clear(); return false;
    }
    output += c;
  }
  return true;
}

bool positiveInt(const std::string& raw, std::int64_t& value)
{
  value = 0;
  if (raw.empty() || raw.size() > 19) return false;
  for (unsigned char c : raw) {
    if (c < '0' || c > '9' ||
        value > (std::numeric_limits<std::int64_t>::max() - (c - '0')) / 10)
      return false;
    value = value * 10 + (c - '0');
  }
  return value > 0;
}

bool boundedInt(const std::string& raw, int max, int& value)
{
  value = 0;
  if (raw.empty() || raw.size() > 3) return false;
  for (unsigned char c : raw) {
    if (c < '0' || c > '9') return false;
    value = value * 10 + (c - '0');
  }
  return value <= max;
}

void append(std::string& fingerprint, const std::string& s)
{
  fingerprint += std::to_string(s.size()) + ":" + s + "|";
}

bool validHhmm(const std::string& s)
{
  if (s.size() != 4) return false;
  for (unsigned char c : s) if (!std::isdigit(c)) return false;
  int hour = (s[0] - '0') * 10 + s[1] - '0';
  int minute = (s[2] - '0') * 10 + s[3] - '0';
  return hour <= 23 && minute <= 59;
}

std::string specificationDigest(
    const SuiteBridgeNativeTimerPreviewVdrRequest& r,
    const std::string& title, const std::string& directory,
    int lifetime)
{
  std::string fingerprint = "native-timer-specification/1|";
  append(fingerprint, r.channelId);
  append(fingerprint, title);
  append(fingerprint, directory);
  append(fingerprint, r.day);
  append(fingerprint, r.weekdays);
  append(fingerprint, r.startTime);
  append(fingerprint, r.endTime);
  append(fingerprint, std::to_string(r.priority));
  append(fingerprint, std::to_string(lifetime));
  append(fingerprint, r.enabled ? "1" : "0");
  append(fingerprint, r.vps ? "1" : "0");
  return vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
      fingerprint);
}

bool parsePreview(const std::vector<std::string>& a,
                  const std::string& currentEpoch,
                  SuiteBridgeNativeTimerPreviewVdrRequest& request,
                  std::string& digest)
{
  // CHECK 1 epoch start stop channelHex titleHex directoryHex dayHex
  // weekdaysHex startHhmmHex stopHhmmHex priority lifetime enabled vps sha256
  if (a.size() != 17 || a[0] != "CHECK" || a[1] != "1" ||
      currentEpoch.empty() || a[2] != currentEpoch ||
      a[2].size() > 128 || !positiveInt(a[3], request.unixStart) ||
      !positiveInt(a[4], request.unixStop) ||
      !decodeHex(a[5], request.channelId, 160))
    return false;
  std::string title, directory;
  if (!decodeHex(a[6], title, 512) ||
      !decodeHex(a[7], directory, 512) ||
      !decodeHex(a[8], request.day, 10) ||
      !decodeHex(a[9], request.weekdays, 7) ||
      !decodeHex(a[10], request.startTime, 4) ||
      !decodeHex(a[11], request.endTime, 4) ||
      !boundedInt(a[12], 99, request.priority))
    return false;
  int lifetime = 0;
  if (!boundedInt(a[13], 99, lifetime) ||
      (a[14] != "0" && a[14] != "1") ||
      (a[15] != "0" && a[15] != "1"))
    return false;
  request.enabled = a[14] == "1";
  request.vps = a[15] == "1";
  if (request.channelId.empty() || title.empty() ||
      request.weekdays != "-------" || !request.enabled ||
      request.vps || request.day.size() != 10 ||
      request.day[4] != '-' || request.day[7] != '-' ||
      !validHhmm(request.startTime) ||
      !validHhmm(request.endTime) ||
      request.unixStop <= request.unixStart ||
      request.unixStop - request.unixStart >= 24 * 3600)
    return false;
  digest = specificationDigest(request, title, directory, lifetime);
  return !digest.empty() && a[16] == digest;
}

std::string snapshotRevision(
    const NativeSnapshot& snapshot, const std::string& epoch,
    const std::string& digest, std::int64_t observedAt)
{
  std::string evidence = "vdr-native-preview-snapshot/1|";
  append(evidence, epoch);
  append(evidence, digest);
  append(evidence, std::to_string(observedAt));
  append(evidence, std::to_string(snapshot.proposedStart));
  append(evidence, std::to_string(snapshot.proposedStop));
  for (const auto& t : snapshot.existingTimers) {
    append(evidence, t.active ? "1" : "0");
    append(evidence, t.exactOneShot ? "1" : "0");
    append(evidence, std::to_string(t.start));
    append(evidence, std::to_string(t.stop));
  }
  for (const auto& d : snapshot.devices) {
    append(evidence, d.enumerated ? "1" : "0");
    append(evidence, d.supportsProposedChannel ? "1" : "0");
    append(evidence, d.availableWithoutPreemptingRecording ? "1" : "0");
  }
  return vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
      evidence);
}

} // namespace

SuiteBridgeNativeTimerPreviewService::SuiteBridgeNativeTimerPreviewService(
    std::string providerEpoch, Capture capture)
    : providerEpoch_(std::move(providerEpoch)),
      capture_(std::move(capture))
{
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerPreviewService::Handle(
    const char* command, const char* option) const
{
  if (!command || strcasecmp(command, "NTPREVIEW") != 0) return {};
  SuiteBridgeNativeTimerPreviewVdrRequest request;
  std::string digest;
  if (!parsePreview(words(option), providerEpoch_, request, digest))
    return reply(501, "vdr-suite-ntpreview-rejected/1 invalid_or_stale");

  if (!capture_)
    return reply(550, "vdr-suite-ntpreview-rejected/1 native_unavailable");

  // A positive result can only arise from the VDR-owned callback. A test
  // fixture returning candidateClear is never a production capability.
  NativeSnapshot snapshot;
  try {
    snapshot = capture_(request);
  } catch (...) {
    return reply(550, "vdr-suite-ntpreview-rejected/1 native_capture_failed");
  }
  const Decision evaluated = evaluate(snapshot);
  if (evaluated.disposition != Disposition::candidateClear)
    return reply(550, std::string("vdr-suite-ntpreview-rejected/1 ") +
        evaluated.reason);
  const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
  if (now <= 0)
    return reply(550, "vdr-suite-ntpreview-rejected/1 native_clock_invalid");
  const std::string revision = snapshotRevision(
      snapshot, providerEpoch_, digest, now);
  if (revision.empty())
    return reply(550, "vdr-suite-ntpreview-rejected/1 evidence_unavailable");
  std::ostringstream result;
  result << "vdr-suite-ntpreview/1 vdr.timer.create 1 suitebridge "
         << providerEpoch_ << " 1 1 " << digest << ' '
         << request.unixStart << ' ' << request.unixStop << ' '
         << now << ' ' << revision << ' '
         << evaluated.consideredTimers << ' '
         << evaluated.consideredDevices << " isolated_clear";
  return reply(900, result.str());
}
