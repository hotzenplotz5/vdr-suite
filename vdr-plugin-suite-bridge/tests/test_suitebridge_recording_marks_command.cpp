#include "suitebridge_recording_marks_command.h"
#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_marks.h"
#include "vdr/recording.h"

#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace FakeVdr {
std::shared_timed_mutex recordingsMutex;
std::atomic<int> activeReaders{0};
int readAttempts = 0;
int requestedTimeoutMs = -1;
bool forceUnavailable = false;
int marksLoads = 0;
bool loadSucceeds = true;
int sequenceCount = 0;
std::string loadedPath;
std::vector<std::pair<int, std::string>> marks;
cRecordings recordings;

void RequireReadLock()
{
  if (activeReaders.load() <= 0)
    throw std::runtime_error("native recording accessed without read lock");
}

void Reset()
{
  if (activeReaders.load() != 0)
    throw std::runtime_error("recordings lock leaked");
  recordings.entries.clear();
  readAttempts = 0;
  requestedTimeoutMs = -1;
  forceUnavailable = false;
  marksLoads = 0;
  loadSucceeds = true;
  sequenceCount = 0;
  loadedPath.clear();
  marks.clear();
}

void SetRecordings(std::vector<cRecording> entries)
{
  recordings.entries = std::move(entries);
  for (std::size_t i = 0; i < recordings.entries.size(); ++i)
    recordings.entries[i].next = i + 1 < recordings.entries.size()
        ? &recordings.entries[i + 1] : nullptr;
}
}

void cStateKey::Remove()
{
  if (locked_) {
    locked_ = false;
    --FakeVdr::activeReaders;
    FakeVdr::recordingsMutex.unlock_shared();
  }
}

const cRecordings *cRecordings::GetRecordingsRead(cStateKey &key, int timeoutMs)
{
  ++FakeVdr::readAttempts;
  FakeVdr::requestedTimeoutMs = timeoutMs;
  if (FakeVdr::forceUnavailable) return nullptr;
  bool acquired = false;
  if (timeoutMs == 0) {
    FakeVdr::recordingsMutex.lock_shared();
    acquired = true;
  } else {
    acquired = FakeVdr::recordingsMutex.try_lock_shared_for(
        std::chrono::milliseconds(timeoutMs));
  }
  if (!acquired) {
    key.timedOut_ = true;
    return nullptr;
  }
  key.locked_ = true;
  ++FakeVdr::activeReaders;
  return &FakeVdr::recordings;
}

const cRecording *cRecordings::First() const
{
  FakeVdr::RequireReadLock();
  return entries.empty() ? nullptr : &entries.front();
}

const cRecording *cRecordings::Next(const cRecording *recording) const
{
  FakeVdr::RequireReadLock();
  return recording->next;
}

bool cMarks::Load(const char *path, double, bool)
{
  // A real marks-file read must never occur while the VDR list lock is held.
  if (FakeVdr::activeReaders.load() != 0)
    throw std::runtime_error("marks file I/O under recordings lock");
  ++FakeVdr::marksLoads;
  FakeVdr::loadedPath = path;
  if (!FakeVdr::loadSucceeds) return false;
  marks_.clear();
  marks_.reserve(FakeVdr::marks.size());
  for (const auto &mark : FakeVdr::marks)
    marks_.emplace_back(mark.first, mark.second);
  for (std::size_t i = 0; i + 1 < marks_.size(); ++i)
    marks_[i].next = &marks_[i + 1];
  return true;
}

namespace {
using Clock = std::chrono::steady_clock;
const std::string path = "/srv/vdr/video/Test/2026-09-04.08.00.1-0.rec";

void require(bool condition, const char *message)
{
  if (!condition) throw std::runtime_error(message);
}

void contains(const std::string &value, const std::string &fragment)
{
  require(value.find(fragment) != std::string::npos, fragment.c_str());
}

cRecording Recording(bool hasMarks = false)
{
  cRecording recording;
  recording.fileName = path;
  recording.hasMarks = hasMarks;
  return recording;
}

std::string Key()
{
  return SuiteBridgeRecordingIdentity::KeyForNativeId(path);
}

SuiteBridgeCommandResult Read()
{
  const std::string key = Key();
  return SuiteBridgeRecordingMarksCommand::Handle("RMARKS", key.c_str());
}

void checkReleased()
{
  require(FakeVdr::activeReaders.load() == 0, "recordings read lock leaked");
  require(FakeVdr::recordingsMutex.try_lock(), "recordings writer cannot acquire after RMARKS");
  FakeVdr::recordingsMutex.unlock();
}

class WriterHold {
public:
  WriterHold() : releaseFuture_(release_.get_future()),
      enteredFuture_(entered_.get_future()), worker_([this] {
        std::unique_lock<std::shared_timed_mutex> lock(FakeVdr::recordingsMutex);
        entered_.set_value();
        releaseFuture_.wait();
      })
  {
    enteredFuture_.wait();
  }
  ~WriterHold()
  {
    release_.set_value();
    worker_.join();
  }
private:
  std::promise<void> entered_;
  std::promise<void> release_;
  std::future<void> releaseFuture_;
  std::future<void> enteredFuture_;
  std::thread worker_;
};

void testRequestValidation()
{
  FakeVdr::Reset();
  const std::string key = Key();
  const auto unrelated = SuiteBridgeRecordingMarksCommand::Handle("RMETA", key.c_str());
  require(!unrelated.handled, "unrelated command handled");
  const auto invalid = SuiteBridgeRecordingMarksCommand::Handle("RMARKS", "invalid");
  require(invalid.handled && invalid.replyCode == 501, "invalid request not rejected");
  require(FakeVdr::readAttempts == 0, "invalid request acquired lock");
}

void testTimedContention()
{
  FakeVdr::Reset();
  FakeVdr::SetRecordings({Recording(true)});
  SuiteBridgeCommandResult result;
  Clock::duration elapsed;
  {
    WriterHold writer;
    const auto start = Clock::now();
    result = Read();
    elapsed = Clock::now() - start;
  }
  require(result.handled && result.replyCode == 451, "contention must fail closed");
  contains(result.payload, "read lock timed out");
  require(FakeVdr::requestedTimeoutMs == 500, "incorrect lock timeout");
  require(elapsed >= std::chrono::milliseconds(350), "lock timeout returned too early");
  require(elapsed < std::chrono::seconds(5), "lock timeout did not bound the wait");
  require(FakeVdr::marksLoads == 0, "marks read attempted after lock timeout");
  checkReleased();
}

void testUnavailable()
{
  FakeVdr::Reset();
  FakeVdr::forceUnavailable = true;
  const auto result = Read();
  require(result.replyCode == 451, "unavailable lock must fail closed");
  contains(result.payload, "read lock unavailable");
  require(FakeVdr::requestedTimeoutMs == 500, "incorrect lock timeout");
  require(FakeVdr::marksLoads == 0, "marks read attempted without lock");
  checkReleased();
}

void testMissingAndAmbiguous()
{
  FakeVdr::Reset();
  auto result = Read();
  require(result.replyCode == 250, "missing recording must use native contract");
  contains(result.payload, "recording_not_found");
  checkReleased();
  FakeVdr::Reset();
  FakeVdr::SetRecordings({Recording(true), Recording(true)});
  result = Read();
  require(result.replyCode == 250, "ambiguous recording must use native contract");
  contains(result.payload, "identity_ambiguous");
  require(FakeVdr::marksLoads == 0, "ambiguous identity accessed marks");
  checkReleased();
}

void testEmptyAndPopulated()
{
  FakeVdr::Reset();
  cRecording recording = Recording();
  recording.pes = true;
  recording.inUse = 3;
  FakeVdr::SetRecordings({recording});
  auto result = Read();
  require(result.replyCode == 250, "empty marks response failed");
  contains(result.payload, "\"state\":\"none\"");
  contains(result.payload, "\"isPesRecording\":true");
  contains(result.payload, "\"inUseFlags\":3");
  contains(result.payload, "\"marks\":[]");
  require(FakeVdr::marksLoads == 0, "missing marks file was read");
  checkReleased();

  FakeVdr::Reset();
  FakeVdr::SetRecordings({Recording(true)});
  FakeVdr::marks = {{100, "begin"}, {250, "end \"quoted\""}};
  FakeVdr::sequenceCount = 1;
  result = Read();
  require(result.replyCode == 250, "populated marks response failed");
  contains(result.payload, "\"state\":\"present\"");
  contains(result.payload, "\"positionFrame\":100");
  contains(result.payload, "\"timecode\":\"00:00:04.00\"");
  contains(result.payload, "\"sequenceCount\":1");
  contains(result.payload, "end \\\"quoted\\\"");
  require(FakeVdr::marksLoads == 1 && FakeVdr::loadedPath == path,
      "marks file was not loaded through the copied native path");
  checkReleased();
}

void testReadFailureAndBounds()
{
  FakeVdr::Reset();
  FakeVdr::SetRecordings({Recording(true)});
  FakeVdr::loadSucceeds = false;
  auto result = Read();
  require(result.replyCode == 250, "unreadable marks response failed");
  contains(result.payload, "\"state\":\"unreadable\"");
  checkReleased();

  FakeVdr::Reset();
  FakeVdr::SetRecordings({Recording(true)});
  FakeVdr::marks = {{100, std::string(SuiteBridgeRecordingMarks::kMaxCommentBytes + 1, 'x')}};
  result = Read();
  require(result.replyCode == 451, "oversized comment accepted");
  contains(result.payload, "comment exceeds contract capacity");
  checkReleased();

  FakeVdr::Reset();
  FakeVdr::SetRecordings({Recording(true)});
  for (std::size_t i = 0; i <= SuiteBridgeRecordingMarks::kMaxMarks; ++i)
    FakeVdr::marks.emplace_back(static_cast<int>(i), "");
  result = Read();
  require(result.replyCode == 451, "oversized marks list accepted");
  contains(result.payload, "marks exceed contract capacity");
  checkReleased();

  FakeVdr::Reset();
  FakeVdr::SetRecordings({Recording(true)});
  for (int i = 0; i < 1000; ++i)
    FakeVdr::marks.emplace_back(i, std::string(32, 'x'));
  result = Read();
  require(result.replyCode == 451, "oversized serialized payload accepted");
  contains(result.payload, "payload exceeds contract capacity");
  checkReleased();
}
}

int main()
{
  try {
    testRequestValidation();
    testTimedContention();
    testUnavailable();
    testMissingAndAmbiguous();
    testEmptyAndPopulated();
    testReadFailureAndBounds();
    std::cout << "RMARKS command regression PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "RMARKS command regression FAIL: " << error.what() << '\n';
    return 1;
  }
}
