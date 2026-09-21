#pragma once

#include <string>
#include <vector>

struct SearchTimerWorkflowBackendReadbackVerificationResult
{
    bool required = false;
    bool attempted = false;
    bool successful = false;
    bool matched = false;
    bool ambiguous = false;
    std::string expectedBackendId;
    std::string expectedBackendNativeId;
    std::string observedBackendNativeId;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<std::string> auditTrail;

    static SearchTimerWorkflowBackendReadbackVerificationResult notRequired(
        const std::string& message = "backend readback not required")
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result;
        result.required = false;
        result.attempted = false;
        result.successful = true;
        result.matched = true;
        result.ambiguous = false;
        result.message = message;
        result.auditTrail.push_back("required=false");
        result.auditTrail.push_back("attempted=false");
        result.auditTrail.push_back("successful=true");
        return result;
    }

    static SearchTimerWorkflowBackendReadbackVerificationResult requiredButUnavailable(
        const std::string& expectedBackendId,
        const std::string& expectedBackendNativeId,
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result;
        result.required = true;
        result.attempted = false;
        result.successful = false;
        result.matched = false;
        result.ambiguous = false;
        result.expectedBackendId = expectedBackendId;
        result.expectedBackendNativeId = expectedBackendNativeId;
        result.message = message;
        result.errors = errors;
        result.auditTrail.push_back("required=true");
        result.auditTrail.push_back("attempted=false");
        result.auditTrail.push_back("successful=false");
        return result;
    }

    static SearchTimerWorkflowBackendReadbackVerificationResult verified(
        const std::string& expectedBackendId,
        const std::string& expectedBackendNativeId,
        const std::string& observedBackendNativeId,
        const std::string& message)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result;
        result.required = true;
        result.attempted = true;
        result.successful = true;
        result.matched = true;
        result.ambiguous = false;
        result.expectedBackendId = expectedBackendId;
        result.expectedBackendNativeId = expectedBackendNativeId;
        result.observedBackendNativeId = observedBackendNativeId;
        result.message = message;
        result.auditTrail.push_back("required=true");
        result.auditTrail.push_back("attempted=true");
        result.auditTrail.push_back("matched=true");
        result.auditTrail.push_back("successful=true");
        return result;
    }

    static SearchTimerWorkflowBackendReadbackVerificationResult failed(
        const std::string& expectedBackendId,
        const std::string& expectedBackendNativeId,
        const std::string& observedBackendNativeId,
        const std::string& message,
        const std::vector<std::string>& errors)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result;
        result.required = true;
        result.attempted = true;
        result.successful = false;
        result.matched = false;
        result.ambiguous = false;
        result.expectedBackendId = expectedBackendId;
        result.expectedBackendNativeId = expectedBackendNativeId;
        result.observedBackendNativeId = observedBackendNativeId;
        result.message = message;
        result.errors = errors;
        result.auditTrail.push_back("required=true");
        result.auditTrail.push_back("attempted=true");
        result.auditTrail.push_back("matched=false");
        result.auditTrail.push_back("successful=false");
        return result;
    }

    static SearchTimerWorkflowBackendReadbackVerificationResult ambiguousMatch(
        const std::string& expectedBackendId,
        const std::string& expectedBackendNativeId,
        const std::string& message,
        const std::vector<std::string>& warnings,
        const std::vector<std::string>& errors)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result;
        result.required = true;
        result.attempted = true;
        result.successful = false;
        result.matched = false;
        result.ambiguous = true;
        result.expectedBackendId = expectedBackendId;
        result.expectedBackendNativeId = expectedBackendNativeId;
        result.message = message;
        result.warnings = warnings;
        result.errors = errors;
        result.auditTrail.push_back("required=true");
        result.auditTrail.push_back("attempted=true");
        result.auditTrail.push_back("ambiguous=true");
        result.auditTrail.push_back("successful=false");
        return result;
    }

    bool passed() const
    {
        if (!required)
        {
            return successful && !hasErrors();
        }

        return attempted &&
               successful &&
               matched &&
               !ambiguous &&
               !hasErrors();
    }

    bool hasWarnings() const
    {
        return !warnings.empty();
    }

    bool hasErrors() const
    {
        return !errors.empty();
    }
};
