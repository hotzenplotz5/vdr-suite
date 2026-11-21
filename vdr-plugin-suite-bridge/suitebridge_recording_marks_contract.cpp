#include "suitebridge_recording_marks_contract.h"

#include "suitebridge_recording_identity.h"

#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace {

bool EqualsIgnoreCase(const char *left, const char *right) noexcept
{
  if (!left || !right) {
    return false;
  }
  while (*left && *right) {
    if (std::toupper(static_cast<unsigned char>(*left)) !=
        std::toupper(static_cast<unsigned char>(*right))) {
      return false;
    }
    ++left;
    ++right;
  }
  return *left == '\0' && *right == '\0';
}

std::string EscapeJson(const std::string &value)
{
  std::string escaped;
  escaped.reserve(value.size());
  static const char hex[] = "0123456789abcdef";

  for (const unsigned char character : value) {
    switch (character) {
    case '"': escaped += "\\\""; break;
    case '\\': escaped += "\\\\"; break;
    case '\b': escaped += "\\b"; break;
    case '\f': escaped += "\\f"; break;
    case '\n': escaped += "\\n"; break;
    case '\r': escaped += "\\r"; break;
    case '\t': escaped += "\\t"; break;
    default:
      if (character < 0x20) {
        escaped += "\\u00";
        escaped += hex[(character >> 4) & 0x0f];
        escaped += hex[character & 0x0f];
      } else {
        escaped += static_cast<char>(character);
      }
      break;
    }
  }
  return escaped;
}

bool MarksValid(const SuiteBridgeRecordingMarks &marks) noexcept
{
  if (!SuiteBridgeRecordingIdentity::IsValidKey(marks.recordingKey) ||
      marks.marks.size() > SuiteBridgeRecordingMarks::kMaxMarks) {
    return false;
  }

  if (!marks.found) {
    return marks.reason != SuiteBridgeRecordingMarksReason::None &&
        marks.marks.empty() && marks.marksRevision.empty();
  }

  if (marks.reason != SuiteBridgeRecordingMarksReason::None ||
      marks.framesPerSecond <= 0.0 ||
      marks.sequenceCount < 0) {
    return false;
  }

  if (marks.state == SuiteBridgeRecordingMarksState::Unreadable) {
    return marks.marks.empty() && marks.marksRevision.empty();
  }

  if (!SuiteBridgeRecordingMarksRevisionValid(marks.marksRevision) ||
      (marks.state == SuiteBridgeRecordingMarksState::None && !marks.marks.empty()) ||
      (marks.state == SuiteBridgeRecordingMarksState::Present && marks.marks.empty())) {
    return false;
  }

  int previous = -1;
  for (const SuiteBridgeRecordingMark &mark : marks.marks) {
    if (mark.positionFrame < 0 ||
        mark.positionFrame < previous ||
        mark.timecode.empty() ||
        mark.comment.size() > SuiteBridgeRecordingMarks::kMaxCommentBytes) {
      return false;
    }
    previous = mark.positionFrame;
  }
  return true;
}

} // namespace

SuiteBridgeRecordingMarksRequest::SuiteBridgeRecordingMarksRequest(
    const char *command,
    const char *option)
{
  handled_ = EqualsIgnoreCase(command, "RMARKS");
  if (!handled_ || !option) {
    return;
  }

  const char *cursor = option;
  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }
  const char *start = cursor;
  while (*cursor && !std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }
  recordingKey_.assign(start, static_cast<std::size_t>(cursor - start));
  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }
  valid_ = *cursor == '\0' &&
      SuiteBridgeRecordingIdentity::IsValidKey(recordingKey_);
}

bool SuiteBridgeRecordingMarksRequest::Handled() const noexcept
{
  return handled_;
}

bool SuiteBridgeRecordingMarksRequest::Valid() const noexcept
{
  return valid_;
}

const std::string &SuiteBridgeRecordingMarksRequest::RecordingKey() const noexcept
{
  return recordingKey_;
}

SuiteBridgeRecordingMarksPayload::SuiteBridgeRecordingMarksPayload(
    const SuiteBridgeRecordingMarks &marks)
{
  if (!MarksValid(marks)) {
    return;
  }

  std::ostringstream stream;
  stream << "{\"schema\":1,\"found\":"
         << (marks.found ? "true" : "false")
         << ",\"reason\":\"" << SuiteBridgeRecordingMarksReasonName(marks.reason)
         << "\",\"recordingIdentitySchema\":1"
         << ",\"recordingKey\":\"" << EscapeJson(marks.recordingKey)
         << "\",\"state\":\"" << SuiteBridgeRecordingMarksStateName(marks.state)
         << "\",\"framesPerSecond\":" << std::setprecision(12) << marks.framesPerSecond
         << ",\"isPesRecording\":" << (marks.isPesRecording ? "true" : "false")
         << ",\"inUseFlags\":" << marks.inUseFlags
         << ",\"marksFilePresent\":" << (marks.marksFilePresent ? "true" : "false")
         << ",\"sequenceCount\":" << marks.sequenceCount
         << ",\"marksRevision\":\"" << EscapeJson(marks.marksRevision)
         << "\",\"marks\":[";

  bool first = true;
  for (const SuiteBridgeRecordingMark &mark : marks.marks) {
    if (!first) {
      stream << ',';
    }
    first = false;
    const double seconds = marks.framesPerSecond > 0.0
        ? static_cast<double>(mark.positionFrame) / marks.framesPerSecond
        : 0.0;
    stream << "{\"positionFrame\":" << mark.positionFrame
           << ",\"timecode\":\"" << EscapeJson(mark.timecode)
           << "\",\"positionSeconds\":" << std::setprecision(12) << seconds
           << ",\"comment\":\"" << EscapeJson(mark.comment) << "\"}";
  }
  stream << "]}";

  const std::string payload = stream.str();
  size_ = payload.size();
  complete_ = size_ < data_.size();
  if (!complete_) {
    size_ = data_.size() - 1;
  }
  std::memcpy(data_.data(), payload.data(), size_);
  data_[size_] = '\0';
}

const char *SuiteBridgeRecordingMarksPayload::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeRecordingMarksPayload::Size() const noexcept
{
  return size_;
}

bool SuiteBridgeRecordingMarksPayload::Complete() const noexcept
{
  return complete_;
}
