#include "MutationOperationIdentity.h"
#include "NativeTimerBindingIdentity.h"

#include <cassert>
#include <set>
#include <string>

int main()
{
    using namespace vdrsuite::operations;
    using namespace vdrsuite::timers;

    std::set<std::string> operationIds;
    std::set<std::string> bindingIds;

    for (int index = 0; index < 16; ++index)
    {
        const std::string operationId = generateMutationOperationId();
        const std::string bindingId = generateNativeTimerBindingId();

        assert(mutationOperationIdCanonical(operationId));
        assert(nativeTimerBindingIdCanonical(bindingId));
        assert(operationIds.insert(operationId).second);
        assert(bindingIds.insert(bindingId).second);
    }

    assert(!mutationOperationIdCanonical(""));
    assert(!mutationOperationIdCanonical("op_"));
    assert(!mutationOperationIdCanonical(
        "op_0123456789abcdef0123456789abcde"));
    assert(!mutationOperationIdCanonical(
        "op_0123456789abcdef0123456789abcdef0"));
    assert(!mutationOperationIdCanonical(
        "op_0123456789ABCDEF0123456789abcdef"));
    assert(!mutationOperationIdCanonical(
        "ntb_0123456789abcdef0123456789abcdef"));

    assert(!nativeTimerBindingIdCanonical(""));
    assert(!nativeTimerBindingIdCanonical("ntb_"));
    assert(!nativeTimerBindingIdCanonical(
        "ntb_0123456789abcdef0123456789abcde"));
    assert(!nativeTimerBindingIdCanonical(
        "ntb_0123456789abcdef0123456789abcdef0"));
    assert(!nativeTimerBindingIdCanonical(
        "ntb_0123456789ABCDEF0123456789abcdef"));
    assert(!nativeTimerBindingIdCanonical(
        "op_0123456789abcdef0123456789abcdef"));

    // These legacy-compatible shapes deliberately remain outside the new
    // canonical issuance contract rather than retroactively becoming invalid
    // durable-domain values.
    assert(!mutationOperationIdCanonical("op_create_dispatch_1"));
    assert(!nativeTimerBindingIdCanonical("native-timer-binding:1"));

    return 0;
}
