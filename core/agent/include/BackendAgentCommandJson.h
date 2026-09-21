#pragma once

#include "BackendAgentCommand.h"

#include <string>

bool parseBackendAgentCommandPollRequestJson(
    const std::string& body,
    BackendAgentCommandPollRequest& request,
    std::string& reasonCode);
std::string serializeBackendAgentCommandPollRequestJson(
    const BackendAgentCommandPollRequest& request);

bool parseBackendAgentCommandPollResponseJson(
    const std::string& body,
    BackendAgentCommandPollResult& result,
    std::string& reasonCode);
std::string serializeBackendAgentCommandPollResponseJson(
    const BackendAgentCommandPollResult& result);

bool parseBackendAgentCommandReceiptJson(
    const std::string& body,
    BackendAgentCommandReceipt& receipt,
    std::string& reasonCode);
std::string serializeBackendAgentCommandReceiptJson(
    const BackendAgentCommandReceipt& receipt);

bool parseBackendAgentCommandResultJson(
    const std::string& body,
    BackendAgentCommandResult& result,
    std::string& reasonCode);
std::string serializeBackendAgentCommandResultJson(
    const BackendAgentCommandResult& result);
