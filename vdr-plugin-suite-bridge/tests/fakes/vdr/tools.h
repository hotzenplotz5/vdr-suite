#ifndef SUITEBRIDGE_TEST_FAKE_TOOLS_H
#define SUITEBRIDGE_TEST_FAKE_TOOLS_H

#include <cstdio>
#include <string>
#include <utility>

class cString {
public:
  explicit cString(std::string value) : value_(std::move(value)) {}
  const char *operator*() const { return value_.c_str(); }
private:
  std::string value_;
};

inline cString IndexToHMSF(int frame, bool, double fps)
{
  if (fps <= 0.0) return cString("");
  const int roundedFps = static_cast<int>(fps + 0.5);
  const int seconds = frame / roundedFps;
  const int remaining = frame % roundedFps;
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%02d",
      seconds / 3600, (seconds / 60) % 60, seconds % 60, remaining);
  return cString(buffer);
}

inline void esyslog(const char *, ...) {}
inline void isyslog(const char *, ...) {}

#endif
