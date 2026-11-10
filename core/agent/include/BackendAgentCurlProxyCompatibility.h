#pragma once

#include <curl/curl.h>

namespace vdrsuite::agent
{
// Only for CURLOPT_PROXY="". A libcurl build without proxy support may
// reject the disabling option, while inherently being unable to use a proxy.
// All other libcurl option errors remain fatal.
constexpr bool proxyDisableOptionAccepted(CURLcode result)
{
    return result == CURLE_OK || result == CURLE_UNKNOWN_OPTION ||
           result == CURLE_NOT_BUILT_IN;
}
}
