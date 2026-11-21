#ifndef SUITEBRIDGE_TEST_FAKE_RECORDING_H
#define SUITEBRIDGE_TEST_FAKE_RECORDING_H

#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

namespace FakeVdr {
extern std::shared_timed_mutex recordingsMutex;
extern std::atomic<int> activeReaders;
extern int readAttempts;
extern int requestedTimeoutMs;
extern bool forceUnavailable;
extern int marksLoads;
extern bool loadSucceeds;
extern int sequenceCount;
extern std::string loadedPath;
extern std::vector<std::pair<int, std::string>> marks;
void RequireReadLock();
}

class cStateKey {
public:
  ~cStateKey() { Remove(); }
  void Remove();
  bool TimedOut() const { return timedOut_; }
  bool timedOut_ = false;
  bool locked_ = false;
};

class cRecording {
public:
  std::string fileName;
  double fps = 25.0;
  bool pes = false;
  int inUse = 0;
  bool hasMarks = false;
  cRecording *next = nullptr;
  const char *FileName() const {
    FakeVdr::RequireReadLock();
    return fileName.c_str();
  }
  double FramesPerSecond() const {
    FakeVdr::RequireReadLock();
    return fps;
  }
  bool IsPesRecording() const {
    FakeVdr::RequireReadLock();
    return pes;
  }
  int IsInUse() const {
    FakeVdr::RequireReadLock();
    return inUse;
  }
  bool HasMarks() const {
    FakeVdr::RequireReadLock();
    return hasMarks;
  }
};

class cRecordings {
public:
  static const cRecordings *GetRecordingsRead(cStateKey &key, int timeoutMs = 0);
  const cRecording *First() const;
  const cRecording *Next(const cRecording *recording) const;
  std::vector<cRecording> entries;
};

namespace FakeVdr {
extern cRecordings recordings;
}

class cMark {
public:
  cMark(int position, std::string comment)
      : position_(position), comment_(std::move(comment)) {}
  int Position() const { return position_; }
  const char *Comment() const { return comment_.c_str(); }
  cMark *next = nullptr;
private:
  int position_;
  std::string comment_;
};

class cMarks {
public:
  bool Load(const char *path, double fps, bool pes);
  int GetNumSequences() const { return FakeVdr::sequenceCount; }
  int Count() const { return static_cast<int>(marks_.size()); }
  cMark *First() { return marks_.empty() ? nullptr : &marks_.front(); }
  cMark *Next(cMark *mark) { return mark->next; }
private:
  std::vector<cMark> marks_;
};

#endif
