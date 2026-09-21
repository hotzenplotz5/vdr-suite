#include "RecordingCacheRefreshQueue.h"

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
    queue.request("");
    queue.request("A", 0);
    assert(queue.takePending().empty());
}
