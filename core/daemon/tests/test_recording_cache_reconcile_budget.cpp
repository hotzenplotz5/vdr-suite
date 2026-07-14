#include "RecordingCacheReconcileBudget.h"

#include <cassert>


int main()
{
    RecordingCacheReconcileBudget budget;

    assert(budget.load() == 0);

    budget.store(8);
    assert(
        budget.load() ==
        RecordingCacheReconcileBudget::maximumAttempts);

    int expected = 2;
    assert(budget.compare_exchange_weak(expected, 1));
    assert(budget.load() == 1);

    expected = 1;
    assert(budget.compare_exchange_weak(expected, 0));
    assert(budget.load() == 0);

    budget.store(-5);
    assert(budget.load() == 0);

    return 0;
}
