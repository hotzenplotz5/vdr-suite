#pragma once

#include <string>

namespace vdrsuite::operations
{

// Canonical Control-Plane identity for newly issued ADR-0042 operations.
// Existing durable operation IDs remain compatibility-valid under their
// established domain contract; this helper defines only new Suite issuance.
std::string generateMutationOperationId();
bool mutationOperationIdCanonical(const std::string& value);

} // namespace vdrsuite::operations
