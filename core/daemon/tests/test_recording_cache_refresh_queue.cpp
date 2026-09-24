#include "RecordingCacheRefreshQueue.h"
#include "RecordingPresentationChangeQueue.h"

#include <cassert>
#include <thread>

int main()
{
    RecordingCacheRefreshQueue queue;
    queue.request("A");
    queue.request("A");
    assert(queue.takePending() == std::vector<std::string>{"A"});
    assert(queue.takePending().empty());
    // A hint arriving while A is being read must survive for the next pass.
    std::thread incoming([&] { queue.request("A"); queue.request("B"); });
    incoming.join();
    queue.completed("A");
    assert(queue.takeCompleted() == std::set<std::string>{"A"});
    assert(queue.takeCompleted().empty());
    assert((queue.takePending() == std::vector<std::string>{"A", "B"}));
    queue.request("B", 3);
    queue.request("B"); // An ordinary hint must not erase action readback retries.
    for (int i = 0; i < 3; ++i)
        assert(queue.takePending() == std::vector<std::string>{"B"});
    assert(queue.takePending().empty());
    // External import: polling has consumed the native version, the sole
    // cache read times out. Recovery must not require another native change.
    using Clock = RecordingCacheRefreshQueue::Clock;
    const auto start = Clock::time_point{};
    RecordingCacheRefreshQueue recovery;
    recovery.request("A");
    assert(recovery.takePending(start) == std::vector<std::string>{"A"});
    recovery.failed("A", start);
    assert(recovery.takeCompleted().empty());
    assert(recovery.takePending(start + std::chrono::seconds(4)).empty());
    recovery.request("A"); // repeated SSE must not bypass the retry delay
    recovery.request("B");
    assert(recovery.takePending(start) == std::vector<std::string>{"B"});
    auto now = start + std::chrono::seconds(5);
    assert(recovery.takePending(now) == std::vector<std::string>{"A"});
    for (int delay : {10, 20, 40, 60, 60}) {
        recovery.failed("A", now);
        assert(recovery.takeCompleted().empty());
        assert(recovery.takePending(now + std::chrono::seconds(delay - 1)).empty());
        now += std::chrono::seconds(delay);
        assert(recovery.takePending(now) == std::vector<std::string>{"A"});
    }
    recovery.request("A"); // a new change during the successful retry survives
    recovery.completed("A");
    assert(recovery.takeCompleted() == std::set<std::string>{"A"});
    assert(recovery.takePending(now) == std::vector<std::string>{"A"});
    recovery.failed("A", now); // success resets the backoff
    assert(recovery.takePending(now + std::chrono::seconds(4)).empty());
    assert(recovery.takePending(now + std::chrono::seconds(5)) == std::vector<std::string>{"A"});
    recovery.completed("A");
    assert(recovery.takePending(now + std::chrono::hours(1)).empty());
    recovery.failed("", now);
    assert(recovery.takePending(now + std::chrono::hours(1)).empty());

    queue.request("");
    queue.request("A", 0);
    assert(queue.takePending().empty());

    RecordingPresentationChangeQueue presentation;
    presentation.request("A");
    presentation.request("A");
    std::thread presentationIncoming([&] {
        presentation.request("B");
        presentation.request("A");
    });
    presentationIncoming.join();
    assert(presentation.takePending() ==
        (std::set<std::string>{"A", "B"}));
    assert(presentation.takePending().empty());
    presentation.request("");
    assert(presentation.takePending().empty());
}
