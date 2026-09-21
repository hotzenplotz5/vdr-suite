#include "suitebridge_native_timer_delete_vdr.h"

#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/timers.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr int TimerWriteLockTimeoutMs = 1000;

SuiteBridgeNativeTimerDeleteMutationResult result(
    SuiteBridgeNativeTimerDeleteMutationDisposition disposition,
    const std::string &evidence)
{
  return {disposition, evidence};
}

std::string evidence(const char *reason, const std::string &commandId)
{
  return std::string("ntdel:vdr:") + reason + ":" + commandId;
}

bool parseLocalTimerId(const std::string &value, int &timerId)
{
  if (value.empty() || value.size() > 10) return false;
  errno = 0;
  char *end = nullptr;
  const long parsed = std::strtol(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0' || parsed <= 0 ||
      parsed > std::numeric_limits<int>::max())
    return false;
  timerId = static_cast<int>(parsed);
  return std::to_string(timerId) == value;
}

void appendField(std::string &value, const std::string &field)
{
  value += std::to_string(field.size());
  value += ':';
  value += field;
  value += '|';
}

void appendInteger(std::string &value, long long number)
{
  value += std::to_string(number);
  value += '|';
}

void appendBoolean(std::string &value, bool flag)
{
  value += flag ? "1|" : "0|";
}

std::string hhmm(int value)
{
  char buffer[8] = {0};
  std::snprintf(buffer, sizeof(buffer), "%04d", value);
  return buffer;
}

std::string day(time_t value)
{
  if (value == 0) return {};
  std::tm local{};
  if (localtime_r(&value, &local) == nullptr) return {};
  char buffer[16] = {0};
  if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local) == 0)
    return {};
  return buffer;
}

std::string weekdays(int value)
{
  static constexpr char Names[7] = {'M', 'T', 'W', 'T', 'F', 'S', 'S'};
  std::string result(7, '-');
  for (int index = 0; index < 7; ++index)
    if ((value & (1 << index)) != 0) result[index] = Names[index];
  return result;
}

void splitFile(const char *file, std::string &directory, std::string &title)
{
  const std::string value = file ? file : "";
  const std::size_t separator = value.find_last_of('~');
  if (separator == std::string::npos) {
    directory.clear();
    title = value;
  } else {
    directory = value.substr(0, separator);
    title = value.substr(separator + 1);
  }
}

std::string canonicalObservedState(const cTimer &timer)
{
  if (timer.Channel() == nullptr) return {};
  const cString channel = timer.Channel()->GetChannelID().ToString();
  if (!*channel) return {};

  std::string directory;
  std::string title;
  splitFile(timer.File(), directory, title);

  std::string eventId;
  if (timer.Event() != nullptr)
    eventId = std::to_string(static_cast<unsigned long long>(timer.Event()->EventID()));

  std::string fingerprint = "native-timer-observed-state/1|";
  appendField(fingerprint, *channel);
  appendField(fingerprint, eventId);
  appendField(fingerprint, title);
  appendField(fingerprint, directory);
  appendField(fingerprint, day(timer.Day()));
  appendField(fingerprint, weekdays(timer.WeekDays()));
  appendField(fingerprint, hhmm(timer.Start()));
  appendField(fingerprint, hhmm(timer.Stop()));
  appendInteger(fingerprint, timer.Flags());
  appendInteger(fingerprint, timer.Priority());
  appendInteger(fingerprint, timer.Lifetime());
  appendBoolean(fingerprint, (timer.Flags() & tfActive) != 0);
  appendBoolean(fingerprint, (timer.Flags() & tfVps) != 0);
  appendBoolean(fingerprint, timer.Recording());
  appendBoolean(fingerprint, timer.Pending());
  return fingerprint;
}

std::uint32_t rotateRight(std::uint32_t value, unsigned count)
{
  return (value >> count) | (value << (32U - count));
}

std::string sha256Token(const std::string &input)
{
  static constexpr std::array<std::uint32_t, 64> K = {
      0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
      0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
      0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
      0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
      0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
      0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
      0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
      0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};
  std::vector<std::uint8_t> message(input.begin(), input.end());
  const std::uint64_t bits = static_cast<std::uint64_t>(message.size()) * 8U;
  message.push_back(0x80U);
  while (message.size() % 64U != 56U) message.push_back(0U);
  for (int shift = 56; shift >= 0; shift -= 8)
    message.push_back(static_cast<std::uint8_t>(bits >> shift));

  std::array<std::uint32_t, 8> h = {0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
      0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U};
  for (std::size_t offset = 0; offset < message.size(); offset += 64U) {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) {
      const std::size_t p = offset + i * 4U;
      w[i] = (static_cast<std::uint32_t>(message[p]) << 24U) |
          (static_cast<std::uint32_t>(message[p + 1]) << 16U) |
          (static_cast<std::uint32_t>(message[p + 2]) << 8U) |
          static_cast<std::uint32_t>(message[p + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
      const std::uint32_t s0 = rotateRight(w[i-15],7) ^ rotateRight(w[i-15],18) ^ (w[i-15] >> 3U);
      const std::uint32_t s1 = rotateRight(w[i-2],17) ^ rotateRight(w[i-2],19) ^ (w[i-2] >> 10U);
      w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    std::uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],x=h[7];
    for (std::size_t i = 0; i < 64; ++i) {
      const std::uint32_t s1 = rotateRight(e,6) ^ rotateRight(e,11) ^ rotateRight(e,25);
      const std::uint32_t ch = (e & f) ^ ((~e) & g);
      const std::uint32_t t1 = x + s1 + ch + K[i] + w[i];
      const std::uint32_t s0 = rotateRight(a,2) ^ rotateRight(a,13) ^ rotateRight(a,22);
      const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t t2 = s0 + maj;
      x=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=x;
  }
  static constexpr char Hex[] = "0123456789abcdef";
  std::string token = "sha256:";
  token.reserve(71);
  for (const std::uint32_t word : h)
    for (int shift = 28; shift >= 0; shift -= 4)
      token.push_back(Hex[(word >> shift) & 0x0fU]);
  return token;
}
}

SuiteBridgeNativeTimerDeleteMutationResult
SuiteBridgeNativeTimerDeleteVdrMutationCallback::DeleteTimer(
    const SuiteBridgeNativeTimerDeleteRequest &request)
{
  int timerId = 0;
  if (!parseLocalTimerId(request.backendNativeTimerId, timerId))
    return result(SuiteBridgeNativeTimerDeleteMutationDisposition::RejectedWithoutEffect,
                  evidence("invalid-id", request.commandId));

  try {
    cStateKey stateKey;
    cTimers *timers = cTimers::GetTimersWrite(stateKey, TimerWriteLockTimeoutMs);
    if (timers == nullptr)
      return result(SuiteBridgeNativeTimerDeleteMutationDisposition::RejectedWithoutEffect,
                    evidence("lock-unavailable", request.commandId));

    cTimer *timer = timers->GetById(timerId, nullptr);
    if (timer == nullptr) {
      stateKey.Remove(false);
      return result(SuiteBridgeNativeTimerDeleteMutationDisposition::RejectedWithoutEffect,
                    evidence("not-found", request.commandId));
    }

    const std::string canonical = canonicalObservedState(*timer);
    const std::string liveFingerprint = canonical.empty() ? std::string() : sha256Token(canonical);
    if (liveFingerprint.empty() || liveFingerprint != request.expectedNativeTimerFingerprint) {
      stateKey.Remove(false);
      return result(SuiteBridgeNativeTimerDeleteMutationDisposition::RejectedWithoutEffect,
                    evidence("fingerprint-mismatch", request.commandId));
    }

    if (timer->Recording()) {
      stateKey.Remove(false);
      return result(SuiteBridgeNativeTimerDeleteMutationDisposition::RejectedWithoutEffect,
                    evidence("recording", request.commandId));
    }

    timers->SetExplicitModify();
    timers->Del(timer);
    timers->SetModified();
    stateKey.Remove();
    return result(SuiteBridgeNativeTimerDeleteMutationDisposition::AppliedUnverified,
                  evidence("deleted-unverified", request.commandId));
  } catch (...) {
    return result(SuiteBridgeNativeTimerDeleteMutationDisposition::OutcomeUnknown,
                  evidence("exception", request.commandId));
  }
}
