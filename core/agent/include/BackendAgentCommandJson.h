#pragma once
#include "BackendAgentCommand.h"
#include <string>
bool parseBackendAgentCommandPollRequestJson(const std::string&,BackendAgentCommandPollRequest&,std::string&);
std::string serializeBackendAgentCommandPollRequestJson(const BackendAgentCommandPollRequest&);
bool parseBackendAgentCommandPollResponseJson(const std::string&,BackendAgentCommandPollResult&,std::string&);
std::string serializeBackendAgentCommandPollResponseJson(const BackendAgentCommandPollResult&);
bool parseBackendAgentCommandReceiptJson(const std::string&,BackendAgentCommandReceipt&,std::string&);
std::string serializeBackendAgentCommandReceiptJson(const BackendAgentCommandReceipt&);
bool parseBackendAgentCommandResultJson(const std::string&,BackendAgentCommandResult&,std::string&);
std::string serializeBackendAgentCommandResultJson(const BackendAgentCommandResult&);
