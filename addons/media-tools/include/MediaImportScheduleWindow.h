#pragma once

#include <optional>
#include <string>

namespace vdrsuite::mediatools {

// Pure local-clock window policy for the existing daemon tick; no timer,
// queue, privileges or new lifecycle are introduced here.
struct MediaImportScheduleWindow {
    int beginMinutes = 0;
    int endMinutes = 0;

    static std::optional<MediaImportScheduleWindow> parse(
        const std::string& value) {
        if (value.size() != 11 || value[2] != ':' ||
            value[5] != '-' || value[8] != ':') return std::nullopt;
        for (const int i : {0, 1, 3, 4, 6, 7, 9, 10}) {
            if (value[i] < '0' || value[i] > '9') return std::nullopt;
        }
        const auto at = [&](int index) {
            const int hour = (value[index] - '0') * 10 +
                             (value[index + 1] - '0');
            const int minute = (value[index + 3] - '0') * 10 +
                               (value[index + 4] - '0');
            return hour <= 23 && minute <= 59 ? hour * 60 + minute : -1;
        };
        const int begin = at(0);
        const int end = at(6);
        if (begin < 0 || end < 0 || begin == end) return std::nullopt;
        return MediaImportScheduleWindow{begin, end};
    }

    bool contains(int localMinutes) const {
        if (localMinutes < 0 || localMinutes >= 1440) return false;
        return beginMinutes < endMinutes
            ? localMinutes >= beginMinutes && localMinutes < endMinutes
            : localMinutes >= beginMinutes || localMinutes < endMinutes;
    }
};

} // namespace vdrsuite::mediatools
