#pragma once

#include <string>

class PublicApiRuntime;

namespace vdrsuite {
namespace public_api {

// Discovery body projection only; no separate transport/security/identity owner.
std::string discoveryGetBody(
    const PublicApiRuntime& runtime,
    const std::string& path,
    bool authenticated);

} // namespace public_api
} // namespace vdrsuite
