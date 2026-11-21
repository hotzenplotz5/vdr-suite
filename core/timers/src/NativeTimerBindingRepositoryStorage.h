#pragma once

#include "NativeTimerBinding.h"

#include <cstdint>
#include <string>
#include <vector>

class Database;

namespace vdrsuite::timers::detail
{

bool nativeBindingSelectById(
    Database& database,
    const std::string& id,
    NativeTimerBinding& binding,
    bool& found);

bool nativeBindingSelectByNativeIdentity(
    Database& database,
    const std::string& backendId,
    const std::string& nativeId,
    NativeTimerBinding& binding,
    bool& found);

bool nativeBindingSelectManagedForAssignment(
    Database& database,
    const std::string& assignmentId,
    NativeTimerBinding& binding,
    bool& found);

bool nativeBindingListForAssignment(
    Database& database,
    const std::string& assignmentId,
    std::vector<NativeTimerBinding>& bindings);

bool nativeBindingInsert(
    Database& database,
    const NativeTimerBinding& binding);

bool nativeBindingUpdate(
    Database& database,
    const NativeTimerBinding& binding,
    std::int64_t expectedRevision);

}
