#include "SearchTimerService.h"

#include <iostream>
#include <type_traits>
#include <vector>

int main()
{
    static_assert(
        std::is_default_constructible<SearchTimerService>::value,
        "SearchTimerService must be default constructible");

    using ListReturnType = decltype(
        std::declval<SearchTimerService>().list(
            std::declval<const std::vector<SearchTimer>&>(),
            std::declval<const SearchTimerQuery&>()));

    static_assert(
        std::is_same<ListReturnType, SearchTimerResult>::value,
        "SearchTimerService::list must return SearchTimerResult");

    std::cout << "test_search_timer_service_interface passed" << std::endl;
    return 0;
}