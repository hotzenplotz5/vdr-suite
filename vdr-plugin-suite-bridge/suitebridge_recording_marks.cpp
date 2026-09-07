#include "suitebridge_recording_marks.h"

#include "suitebridge_recording_identity.h"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace {

std::uint64_t Fnv1a64(
    const std::string &value,
    std::uint64_t hash) noexcept
{
  for (const unsigned char character : value) {
    hash ^= static_cast<std::uint64_t>(character);
    hash *= 1099511628211ULL;
  }
  return hash;
}

} // namespace

const char *SuiteBridgeRecordingMarksReasonName(
    SuiteBridgeRecordingMarksReason reason) noexcept
{
  switch (reason) {
  case SuiteBridgeRecordingMarksReason::None:
    return "none";
  case SuiteBridgeRecordingMarksReason::RecordingNotFound:
    return "recording_not_found";
  case SuiteBridgeRecordingMarksReason::IdentityAmbiguous:
    return "identity_ambiguous";
  }
  return "unknown";
}

const char *SuiteBridgeRecordingMarksStateName(
    SuiteBridgeRecordingMarksState state) noexcept
{
  switch (state) {
  case SuiteBridgeRecordingMarksState::None:
    return "none";
  case SuiteBridgeRecordingMarksState::Present:
    return "present";
  case SuiteBridgeRecordingMarksState::Unreadable:
    return "unreadable";
  }
  return "unknown";
}

std::string SuiteBridgeRecordingMarksRevision(
    const SuiteBridgeRecordingMarks &marks)
{
  if (!marks.found ||
      !SuiteBridgeRecordingIdentity::IsValidKey(marks.recordingKey) ||
      marks.framesPerSecond <= 0.0 ||
      marks.state == SuiteBridgeRecordingMarksState::Unreadable ||
      marks.marks.size() > SuiteBridgeRecordingMarks::kMaxMarks) {
    return {};
  }

  std::ostringstream canonical;
  canonical << "vdr-suite-recording-marks-v1\n"
            << marks.recordingKey << '\n'
            << std::setprecision(17) << marks.framesPerSecond << '\n'
            << (marks.isPesRecording ? '1' : '0') << '\n'
            << (marks.marksFilePresent ? '1' : '0') << '\n';
  for (const SuiteBridgeRecordingMark &mark : marks.marks) {
    if (mark.positionFrame < 0 ||
        mark.comment.size() > SuiteBridgeRecordingMarks::kMaxCommentBytes) {
      return {};
    }
    canonical << mark.positionFrame << ':'
              << mark.comment.size() << ':' << mark.comment << '\n';
  }

  const std::string value = canonical.str();
  const std::uint64_t first =
      Fnv1a64(value, 14695981039346656037ULL);
  const std::uint64_t second =
      Fnv1a64(value, 7809847782465536322ULL);

  std::ostringstream revision;
  revision << std::hex << std::nouppercase << std::setfill('0')
           << std::setw(16) << first
           << std::setw(16) << second;
  return revision.str();
}

bool SuiteBridgeRecordingMarksRevisionValid(
    const std::string &revision) noexcept
{
  if (revision.size() != 32) {
    return false;
  }
  for (const unsigned char character : revision) {
    if (!((character >= '0' && character <= '9') ||
          (character >= 'a' && character <= 'f'))) {
      return false;
    }
  }
  return true;
}
