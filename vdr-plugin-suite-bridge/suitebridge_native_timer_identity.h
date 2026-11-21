#pragma once

// RESTfulAPI's VdrExtension::getTimerID uses the tuple:
// channel ID : weekday bitmask : day epoch : HHMM start : HHMM stop.
// Never interpret that string as a VDR numeric timer ID or list position.
#include <vdr/channels.h>
#include <vdr/timers.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <string>

namespace vdrsuite_suitebridge {

inline std::string restfulTimerId(const cTimer& timer)
{
  if (!timer.Channel() || timer.Remote()) return {};
  const cString channel = timer.Channel()->GetChannelID().ToString();
  if (!*channel) return {};
  std::ostringstream out;
  out << *channel << ':' << timer.WeekDays() << ':' << timer.Day()
      << ':' << timer.Start() << ':' << timer.Stop();
  return out.str();
}

// Must be called ONLY while holding cTimers::GetTimersWrite(). Ambiguous
// RESTfulAPI identities are rejected, never resolved by first-match.
inline cTimer* exactLocalTimer(cTimers& timers, const std::string& identity)
{
  if (identity.empty() || identity.size() > 160) return nullptr;
  if (identity.find(':') == std::string::npos)
  {
    if (identity[0] == '0') return nullptr;
    errno = 0;
    char* end = nullptr;
    const long numeric = std::strtol(identity.c_str(), &end, 10);
    if (errno || !end || *end || numeric <= 0 || numeric > INT_MAX ||
        std::to_string(numeric) != identity)
      return nullptr;
    return timers.GetById(static_cast<int>(numeric), nullptr);
  }
  cTimer* match = nullptr;
  for (cTimer* timer = timers.First(); timer; timer = timers.Next(timer))
  {
    if (restfulTimerId(*timer) == identity)
    {
      if (match) return nullptr;
      match = timer;
    }
  }
  return match;
}

} // namespace vdrsuite_suitebridge
