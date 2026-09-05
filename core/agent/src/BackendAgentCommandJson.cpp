#include "BackendAgentCommandJson.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerModify.h"
#include "BackendAgentRecordingMarksModify.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr std::size_t MaximumBodyBytes = 64U * 1024U;
constexpr std::size_t MaximumStringBytes = 4096U;
constexpr std::size_t MaximumArrayItems = 32U;
constexpr int MaximumDepth = 4;

enum class Kind { String, Unsigned, Boolean, Object, Array };
struct Value
{
    Kind kind = Kind::String;
    std::string stringValue;
    std::uint64_t unsignedValue = 0;
    bool boolValue = false;
    std::map<std::string, Value> objectValue;
    std::vector<Value> arrayValue;
};

bool validUtf8(const std::string& value)
{
    std::size_t index = 0;
    while (index < value.size())
    {
        const unsigned char first = static_cast<unsigned char>(value[index]);
        if (first <= 0x7fU) { ++index; continue; }
        std::size_t count = 0;
        std::uint32_t codePoint = 0;
        if ((first & 0xe0U) == 0xc0U) { count = 2; codePoint = first & 0x1fU; }
        else if ((first & 0xf0U) == 0xe0U) { count = 3; codePoint = first & 0x0fU; }
        else if ((first & 0xf8U) == 0xf0U) { count = 4; codePoint = first & 0x07U; }
        else return false;
        if (index + count > value.size()) return false;
        for (std::size_t offset = 1; offset < count; ++offset)
        {
            const unsigned char next = static_cast<unsigned char>(value[index + offset]);
            if ((next & 0xc0U) != 0x80U) return false;
            codePoint = (codePoint << 6U) | (next & 0x3fU);
        }
        if ((count == 2 && codePoint < 0x80U) ||
            (count == 3 && codePoint < 0x800U) ||
            (count == 4 && codePoint < 0x10000U) ||
            codePoint > 0x10ffffU ||
            (codePoint >= 0xd800U && codePoint <= 0xdfffU)) return false;
        index += count;
    }
    return true;
}

class Parser
{
public:
    explicit Parser(const std::string& input) : input_(input) {}
    bool parse(Value& value)
    {
        if (input_.empty() || input_.size() > MaximumBodyBytes) return false;
        position_ = skip(position_);
        if (!parseValue(value, 0)) return false;
        position_ = skip(position_);
        return position_ == input_.size();
    }
private:
    std::size_t skip(std::size_t value) const
    {
        while (value < input_.size() && std::isspace(static_cast<unsigned char>(input_[value]))) ++value;
        return value;
    }
    bool parseValue(Value& value, int depth)
    {
        if (depth > MaximumDepth || position_ >= input_.size()) return false;
        if (input_[position_] == '"') { value.kind = Kind::String; return parseString(value.stringValue); }
        if (input_[position_] == '{') return parseObject(value, depth + 1);
        if (input_[position_] == '[') return parseArray(value, depth + 1);
        if (input_.compare(position_, 4, "true") == 0) { value.kind = Kind::Boolean; value.boolValue = true; position_ += 4; return true; }
        if (input_.compare(position_, 5, "false") == 0) { value.kind = Kind::Boolean; value.boolValue = false; position_ += 5; return true; }
        return parseUnsigned(value);
    }
    bool parseString(std::string& value)
    {
        value.clear();
        if (input_[position_] != '"') return false;
        ++position_;
        while (position_ < input_.size())
        {
            const unsigned char character = static_cast<unsigned char>(input_[position_++]);
            if (character == '"') return validUtf8(value);
            if (character < 0x20U) return false;
            if (character == '\\')
            {
                if (position_ >= input_.size()) return false;
                const char escaped = input_[position_++];
                switch (escaped)
                {
                    case '"': value.push_back('"'); break;
                    case '\\': value.push_back('\\'); break;
                    case '/': value.push_back('/'); break;
                    case 'b': value.push_back('\b'); break;
                    case 'f': value.push_back('\f'); break;
                    case 'n': value.push_back('\n'); break;
                    case 'r': value.push_back('\r'); break;
                    case 't': value.push_back('\t'); break;
                    default: return false;
                }
            }
            else value.push_back(static_cast<char>(character));
            if (value.size() > MaximumStringBytes) return false;
        }
        return false;
    }
    bool parseUnsigned(Value& value)
    {
        const std::size_t start = position_;
        while (position_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_;
        if (start == position_ || position_ - start > 19 || (position_ - start > 1 && input_[start] == '0')) return false;
        std::uint64_t number = 0;
        for (std::size_t index = start; index < position_; ++index)
        {
            const unsigned digit = static_cast<unsigned>(input_[index] - '0');
            if (number > (static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - digit) / 10U) return false;
            number = number * 10U + digit;
        }
        value.kind = Kind::Unsigned;
        value.unsignedValue = number;
        return true;
    }
    bool parseObject(Value& value, int depth)
    {
        value.kind = Kind::Object;
        value.objectValue.clear();
        ++position_;
        position_ = skip(position_);
        if (position_ < input_.size() && input_[position_] == '}') { ++position_; return true; }
        while (position_ < input_.size())
        {
            std::string key;
            if (!parseString(key) || key.empty() || key.size() > 128) return false;
            position_ = skip(position_);
            if (position_ >= input_.size() || input_[position_] != ':') return false;
            ++position_;
            position_ = skip(position_);
            Value child;
            if (!parseValue(child, depth)) return false;
            if (!value.objectValue.emplace(key, std::move(child)).second || value.objectValue.size() > 32U) return false;
            position_ = skip(position_);
            if (position_ >= input_.size()) return false;
            if (input_[position_] == '}') { ++position_; return true; }
            if (input_[position_] != ',') return false;
            ++position_;
            position_ = skip(position_);
        }
        return false;
    }
    bool parseArray(Value& value, int depth)
    {
        value.kind = Kind::Array;
        value.arrayValue.clear();
        ++position_;
        position_ = skip(position_);
        if (position_ < input_.size() && input_[position_] == ']') { ++position_; return true; }
        while (position_ < input_.size())
        {
            Value child;
            if (!parseValue(child, depth)) return false;
            value.arrayValue.push_back(std::move(child));
            if (value.arrayValue.size() > MaximumArrayItems) return false;
            position_ = skip(position_);
            if (position_ >= input_.size()) return false;
            if (input_[position_] == ']') { ++position_; return true; }
            if (input_[position_] != ',') return false;
            ++position_;
            position_ = skip(position_);
        }
        return false;
    }
    const std::string& input_;
    std::size_t position_ = 0;
};

bool exactKeys(const std::map<std::string, Value>& object, const std::vector<std::string>& keys)
{
    if (object.size() != keys.size()) return false;
    return std::all_of(keys.begin(), keys.end(), [&](const std::string& key) { return object.count(key) == 1; });
}
const Value* get(const std::map<std::string, Value>& object, const std::string& key, Kind kind)
{
    const auto found = object.find(key);
    return found != object.end() && found->second.kind == kind ? &found->second : nullptr;
}
std::string escape(const std::string& value)
{
    std::ostringstream out;
    for (unsigned char character : value)
    {
        switch (character)
        {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: if (character >= 0x20U) out << static_cast<char>(character);
        }
    }
    return out.str();
}
std::string stringArray(const std::vector<std::string>& values)
{
    std::ostringstream out;
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0) out << ',';
        out << '"' << escape(values[index]) << '"';
    }
    out << ']';
    return out.str();
}
bool parseIdentifiers(const Value& value, std::vector<std::string>& identifiers)
{
    if (value.kind != Kind::Array || value.arrayValue.empty() ||
        value.arrayValue.size() > 64) return false;
    identifiers.clear();
    for (const Value& item : value.arrayValue)
    {
        if (item.kind != Kind::String ||
            !backendAgentCommandSafeIdentifier(item.stringValue) ||
            std::find(identifiers.begin(), identifiers.end(), item.stringValue) !=
                identifiers.end()) return false;
        identifiers.push_back(item.stringValue);
    }
    return true;
}
bool parseTypes(const Value& value, std::vector<std::string>& types)
{
    if (value.kind != Kind::Array || value.arrayValue.size() > 8) return false;
    types.clear();
    for (const Value& item : value.arrayValue)
    {
        if (item.kind != Kind::String ||
            (item.stringValue != "probe.noop" &&
             item.stringValue != "vdr.native.probe" &&
             item.stringValue !=
                 vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType &&
             item.stringValue !=
                 vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType &&
             item.stringValue !=
                 vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType &&
             item.stringValue !=
                 vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType &&
             item.stringValue !=
                 vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType) ||
            std::find(types.begin(), types.end(), item.stringValue) != types.end())
        {
            return false;
        }
        types.push_back(item.stringValue);
    }
    return true;
}

bool parseProviderFacts(
    const Value& value,
    vdrsuite::agent::BackendAgentLocalProviderFacts& facts)
{
    if (value.kind != Kind::Object || !exactKeys(value.objectValue, {
        "providerId", "providerKind", "providerInstanceEpoch",
        "providerGeneration", "capabilityRevision", "available",
        "capabilities"})) return false;
    const Value *providerId=get(value.objectValue,"providerId",Kind::String),
        *providerKind=get(value.objectValue,"providerKind",Kind::String),
        *epoch=get(value.objectValue,"providerInstanceEpoch",Kind::String),
        *generation=get(value.objectValue,"providerGeneration",Kind::Unsigned),
        *revision=get(value.objectValue,"capabilityRevision",Kind::Unsigned),
        *available=get(value.objectValue,"available",Kind::Boolean),
        *capabilities=get(value.objectValue,"capabilities",Kind::Array);
    if (!providerId || !providerKind || !epoch || !generation || !revision ||
        !available || !capabilities ||
        !parseIdentifiers(*capabilities, facts.capabilities)) return false;
    facts.providerId = providerId->stringValue;
    facts.providerKind = providerKind->stringValue;
    facts.providerInstanceEpoch = epoch->stringValue;
    facts.providerGeneration = generation->unsignedValue;
    facts.capabilityRevision = revision->unsignedValue;
    facts.available = available->boolValue;
    return vdrsuite::agent::backendAgentLocalProviderValidFacts(facts);
}

bool parseProviders(
    const Value& value,
    std::vector<vdrsuite::agent::BackendAgentLocalProviderFacts>& providers)
{
    if (value.kind != Kind::Array || value.arrayValue.size() > 16) return false;
    providers.clear();
    for (const Value& item : value.arrayValue)
    {
        vdrsuite::agent::BackendAgentLocalProviderFacts facts;
        if (!parseProviderFacts(item, facts) ||
            std::any_of(providers.begin(), providers.end(), [&](const auto& existing) {
                return existing.providerId == facts.providerId;
            })) return false;
        providers.push_back(std::move(facts));
    }
    return true;
}

std::string providerFactsJson(
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts)
{
    std::ostringstream out;
    out << "{\"providerId\":\"" << escape(facts.providerId)
        << "\",\"providerKind\":\"" << escape(facts.providerKind)
        << "\",\"providerInstanceEpoch\":\""
        << escape(facts.providerInstanceEpoch)
        << "\",\"providerGeneration\":" << facts.providerGeneration
        << ",\"capabilityRevision\":" << facts.capabilityRevision
        << ",\"available\":" << (facts.available ? "true" : "false")
        << ",\"capabilities\":" << stringArray(facts.capabilities) << '}';
    return out.str();
}

std::string providersJson(
    const std::vector<vdrsuite::agent::BackendAgentLocalProviderFacts>& providers)
{
    std::ostringstream out;
    out << '[';
    for (std::size_t index = 0; index < providers.size(); ++index)
    {
        if (index != 0) out << ',';
        out << providerFactsJson(providers[index]);
    }
    out << ']';
    return out.str();
}

bool readAssignment(const Value& value, BackendAgentCommandAssignment& assignment)
{
    if (value.kind != Kind::Object || !exactKeys(value.objectValue, {
        "protocolVersion", "requestId", "correlationId", "operationId", "jobId",
        "attemptId", "claimEpoch", "commandId", "backendId", "agentId",
        "agentInstanceId", "backendGeneration", "commandType", "payloadVersion",
        "payload", "requestFingerprint", "verificationPolicy", "assignedAt", "deadline"})) return false;
    const auto s = [&](const char* key) { return get(value.objectValue, key, Kind::String); };
    const auto u = [&](const char* key) { return get(value.objectValue, key, Kind::Unsigned); };
    const Value *protocol=s("protocolVersion"), *request=s("requestId"), *correlation=s("correlationId"),
        *operation=s("operationId"), *job=s("jobId"), *attempt=s("attemptId"), *claim=u("claimEpoch"),
        *command=s("commandId"), *backend=s("backendId"), *agent=s("agentId"), *instance=s("agentInstanceId"),
        *generation=u("backendGeneration"), *type=s("commandType"), *payloadVersion=u("payloadVersion"),
        *payload=s("payload"), *fingerprint=s("requestFingerprint"), *verification=s("verificationPolicy"),
        *assigned=u("assignedAt"), *deadline=u("deadline");
    if (!protocol||!request||!correlation||!operation||!job||!attempt||!claim||!command||!backend||!agent||!instance||
        !generation||!type||!payloadVersion||!payload||!fingerprint||!verification||!assigned||!deadline) return false;
    assignment.present = true;
    assignment.protocolVersion=protocol->stringValue; assignment.requestId=request->stringValue;
    assignment.correlationId=correlation->stringValue; assignment.operationId=operation->stringValue;
    assignment.jobId=job->stringValue; assignment.attemptId=attempt->stringValue;
    assignment.claimEpoch=claim->unsignedValue; assignment.commandId=command->stringValue;
    assignment.backendId=backend->stringValue; assignment.agentId=agent->stringValue;
    assignment.agentInstanceId=instance->stringValue; assignment.backendGeneration=generation->unsignedValue;
    assignment.commandType=type->stringValue; assignment.payloadVersion=payloadVersion->unsignedValue;
    assignment.payload=payload->stringValue; assignment.requestFingerprint=fingerprint->stringValue;
    assignment.verificationPolicy=verification->stringValue;
    assignment.assignedAt=static_cast<std::int64_t>(assigned->unsignedValue);
    assignment.deadline=static_cast<std::int64_t>(deadline->unsignedValue);
    return backendAgentCommandValidAssignment(assignment);
}

std::string assignmentJson(const BackendAgentCommandAssignment& a)
{
    std::ostringstream out;
    out << "{\"protocolVersion\":\"" << escape(a.protocolVersion)
        << "\",\"requestId\":\"" << escape(a.requestId)
        << "\",\"correlationId\":\"" << escape(a.correlationId)
        << "\",\"operationId\":\"" << escape(a.operationId)
        << "\",\"jobId\":\"" << escape(a.jobId)
        << "\",\"attemptId\":\"" << escape(a.attemptId)
        << "\",\"claimEpoch\":" << a.claimEpoch
        << ",\"commandId\":\"" << escape(a.commandId)
        << "\",\"backendId\":\"" << escape(a.backendId)
        << "\",\"agentId\":\"" << escape(a.agentId)
        << "\",\"agentInstanceId\":\"" << escape(a.agentInstanceId)
        << "\",\"backendGeneration\":" << a.backendGeneration
        << ",\"commandType\":\"" << escape(a.commandType)
        << "\",\"payloadVersion\":" << a.payloadVersion
        << ",\"payload\":\"" << escape(a.payload)
        << "\",\"requestFingerprint\":\"" << escape(a.requestFingerprint)
        << "\",\"verificationPolicy\":\"" << escape(a.verificationPolicy)
        << "\",\"assignedAt\":" << a.assignedAt
        << ",\"deadline\":" << a.deadline << '}';
    return out.str();
}
}

bool parseBackendAgentCommandPollRequestJson(const std::string& body, BackendAgentCommandPollRequest& request, std::string& reason)
{
    Value root;
    if (!Parser(body).parse(root) || root.kind != Kind::Object)
    { reason="invalid_command_poll_payload"; return false; }
    const bool legacy = exactKeys(root.objectValue,
        {"protocolVersion", "backendId", "agentInstanceId", "backendGeneration", "supportedCommandTypes"});
    const bool current = exactKeys(root.objectValue,
        {"protocolVersion", "backendId", "agentInstanceId", "backendGeneration", "supportedCommandTypes", "localProviders"});
    if (!legacy && !current)
    { reason="invalid_command_poll_payload"; return false; }
    const Value* protocol=get(root.objectValue,"protocolVersion",Kind::String);
    const Value* backend=get(root.objectValue,"backendId",Kind::String);
    const Value* instance=get(root.objectValue,"agentInstanceId",Kind::String);
    const Value* generation=get(root.objectValue,"backendGeneration",Kind::Unsigned);
    const Value* types=get(root.objectValue,"supportedCommandTypes",Kind::Array);
    if (!protocol||!backend||!instance||!generation||!types||
        !parseTypes(*types,request.supportedCommandTypes))
    { reason="invalid_command_poll_payload"; return false; }
    request.localProviders.clear();
    if (current)
    {
        const Value* providers=get(root.objectValue,"localProviders",Kind::Array);
        if (!providers || !parseProviders(*providers, request.localProviders))
        { reason="invalid_command_poll_payload"; return false; }
    }
    request.protocolVersion=protocol->stringValue; request.backendId=backend->stringValue;
    request.agentInstanceId=instance->stringValue; request.backendGeneration=generation->unsignedValue;
    if (request.protocolVersion!="vdr-suite-agent/1" || !backendAgentCommandSafeIdentifier(request.backendId) ||
        !backendAgentCommandSafeIdentifier(request.agentInstanceId) || request.backendGeneration==0)
    { reason="invalid_command_poll_payload"; return false; }
    reason="command_poll_parsed"; return true;
}

std::string serializeBackendAgentCommandPollRequestJson(const BackendAgentCommandPollRequest& request)
{
    std::ostringstream out;
    out << "{\"protocolVersion\":\"vdr-suite-agent/1\",\"backendId\":\"" << escape(request.backendId)
        << "\",\"agentInstanceId\":\"" << escape(request.agentInstanceId)
        << "\",\"backendGeneration\":" << request.backendGeneration
        << ",\"supportedCommandTypes\":" << stringArray(request.supportedCommandTypes)
        << ",\"localProviders\":" << providersJson(request.localProviders) << '}';
    return out.str();
}

bool parseBackendAgentCommandPollResponseJson(const std::string& body, BackendAgentCommandPollResult& result, std::string& reason)
{
    Value root;
    if (!Parser(body).parse(root) || root.kind!=Kind::Object) { reason="invalid_command_poll_response"; return false; }
    const Value* has=get(root.objectValue,"hasAssignment",Kind::Boolean);
    const Value* code=get(root.objectValue,"reasonCode",Kind::String);
    if (!has||!code) { reason="invalid_command_poll_response"; return false; }
    if (!has->boolValue)
    {
        if (!exactKeys(root.objectValue,{"hasAssignment","reasonCode"})) { reason="invalid_command_poll_response"; return false; }
        result.accepted=true; result.reasonCode=code->stringValue; result.assignment={}; reason="command_poll_response_parsed"; return true;
    }
    if (!exactKeys(root.objectValue,{"hasAssignment","reasonCode","assignment"})) { reason="invalid_command_poll_response"; return false; }
    const Value* assignment=get(root.objectValue,"assignment",Kind::Object);
    if (!assignment||!readAssignment(*assignment,result.assignment)) { reason="invalid_command_assignment"; return false; }
    result.accepted=true; result.reasonCode=code->stringValue; reason="command_poll_response_parsed"; return true;
}

std::string serializeBackendAgentCommandPollResponseJson(const BackendAgentCommandPollResult& result)
{
    std::ostringstream out;
    out << "{\"hasAssignment\":" << (result.assignment.present?"true":"false")
        << ",\"reasonCode\":\"" << escape(result.reasonCode) << '"';
    if (result.assignment.present) out << ",\"assignment\":" << assignmentJson(result.assignment);
    out << '}';
    return out.str();
}

bool parseBackendAgentCommandReceiptJson(const std::string& body, BackendAgentCommandReceipt& receipt, std::string& reason)
{
    Value root;
    if (!Parser(body).parse(root)||root.kind!=Kind::Object||!exactKeys(root.objectValue,
        {"protocolVersion","commandId","requestFingerprint","jobId","attemptId","claimEpoch","backendId","agentId","agentInstanceId","backendGeneration","receiptCategory","receivedAt","reasonCode"}))
    { reason="invalid_command_receipt_payload"; return false; }
    const auto s=[&](const char* key){return get(root.objectValue,key,Kind::String);};
    const auto u=[&](const char* key){return get(root.objectValue,key,Kind::Unsigned);};
    const Value *protocol=s("protocolVersion"),*command=s("commandId"),*finger=s("requestFingerprint"),*job=s("jobId"),
        *attempt=s("attemptId"),*claim=u("claimEpoch"),*backend=s("backendId"),*agent=s("agentId"),*instance=s("agentInstanceId"),
        *generation=u("backendGeneration"),*category=s("receiptCategory"),*received=u("receivedAt"),*code=s("reasonCode");
    if (!protocol||!command||!finger||!job||!attempt||!claim||!backend||!agent||!instance||!generation||!category||!received||!code)
    { reason="invalid_command_receipt_payload"; return false; }
    receipt.protocolVersion=protocol->stringValue; receipt.commandId=command->stringValue; receipt.requestFingerprint=finger->stringValue;
    receipt.jobId=job->stringValue; receipt.attemptId=attempt->stringValue; receipt.claimEpoch=claim->unsignedValue;
    receipt.backendId=backend->stringValue; receipt.agentId=agent->stringValue; receipt.agentInstanceId=instance->stringValue;
    receipt.backendGeneration=generation->unsignedValue; receipt.receiptCategory=category->stringValue;
    receipt.receivedAt=static_cast<std::int64_t>(received->unsignedValue); receipt.reasonCode=code->stringValue;
    if (!backendAgentCommandValidReceipt(receipt)) { reason="invalid_command_receipt_payload"; return false; }
    reason="command_receipt_parsed"; return true;
}

std::string serializeBackendAgentCommandReceiptJson(const BackendAgentCommandReceipt& r)
{
    std::ostringstream out;
    out << "{\"protocolVersion\":\"vdr-suite-agent/1\",\"commandId\":\""<<escape(r.commandId)
        <<"\",\"requestFingerprint\":\""<<escape(r.requestFingerprint)<<"\",\"jobId\":\""<<escape(r.jobId)
        <<"\",\"attemptId\":\""<<escape(r.attemptId)<<"\",\"claimEpoch\":"<<r.claimEpoch
        <<",\"backendId\":\""<<escape(r.backendId)<<"\",\"agentId\":\""<<escape(r.agentId)
        <<"\",\"agentInstanceId\":\""<<escape(r.agentInstanceId)<<"\",\"backendGeneration\":"<<r.backendGeneration
        <<",\"receiptCategory\":\""<<escape(r.receiptCategory)<<"\",\"receivedAt\":"<<r.receivedAt
        <<",\"reasonCode\":\""<<escape(r.reasonCode)<<"\"}";
    return out.str();
}

bool parseBackendAgentCommandResultJson(const std::string& body, BackendAgentCommandResult& result, std::string& reason)
{
    Value root;
    if (!Parser(body).parse(root)||root.kind!=Kind::Object||!exactKeys(root.objectValue,
        {"protocolVersion","commandId","requestFingerprint","jobId","attemptId","claimEpoch","backendId","agentId","agentInstanceId","backendGeneration","dispatchState","verificationState","resultCategory","errorCategory","retryClassification","boundedDiagnostics","completedAt"}))
    { reason="invalid_command_result_payload"; return false; }
    const auto s=[&](const char* key){return get(root.objectValue,key,Kind::String);};
    const auto u=[&](const char* key){return get(root.objectValue,key,Kind::Unsigned);};
    const Value *protocol=s("protocolVersion"),*command=s("commandId"),*finger=s("requestFingerprint"),*job=s("jobId"),
        *attempt=s("attemptId"),*claim=u("claimEpoch"),*backend=s("backendId"),*agent=s("agentId"),*instance=s("agentInstanceId"),
        *generation=u("backendGeneration"),*dispatch=s("dispatchState"),*verification=s("verificationState"),*category=s("resultCategory"),
        *error=s("errorCategory"),*retry=s("retryClassification"),*diagnostics=s("boundedDiagnostics"),*completed=u("completedAt");
    if (!protocol||!command||!finger||!job||!attempt||!claim||!backend||!agent||!instance||!generation||!dispatch||!verification||!category||!error||!retry||!diagnostics||!completed)
    { reason="invalid_command_result_payload"; return false; }
    result.protocolVersion=protocol->stringValue; result.commandId=command->stringValue; result.requestFingerprint=finger->stringValue;
    result.jobId=job->stringValue; result.attemptId=attempt->stringValue; result.claimEpoch=claim->unsignedValue;
    result.backendId=backend->stringValue; result.agentId=agent->stringValue; result.agentInstanceId=instance->stringValue;
    result.backendGeneration=generation->unsignedValue; result.dispatchState=dispatch->stringValue; result.verificationState=verification->stringValue;
    result.resultCategory=category->stringValue; result.errorCategory=error->stringValue; result.retryClassification=retry->stringValue;
    result.boundedDiagnostics=diagnostics->stringValue; result.completedAt=static_cast<std::int64_t>(completed->unsignedValue);
    if (!backendAgentCommandValidResult(result)) { reason="invalid_command_result_payload"; return false; }
    reason="command_result_parsed"; return true;
}

std::string serializeBackendAgentCommandResultJson(const BackendAgentCommandResult& r)
{
    std::ostringstream out;
    out << "{\"protocolVersion\":\"vdr-suite-agent/1\",\"commandId\":\""<<escape(r.commandId)
        <<"\",\"requestFingerprint\":\""<<escape(r.requestFingerprint)<<"\",\"jobId\":\""<<escape(r.jobId)
        <<"\",\"attemptId\":\""<<escape(r.attemptId)<<"\",\"claimEpoch\":"<<r.claimEpoch
        <<",\"backendId\":\""<<escape(r.backendId)<<"\",\"agentId\":\""<<escape(r.agentId)
        <<"\",\"agentInstanceId\":\""<<escape(r.agentInstanceId)<<"\",\"backendGeneration\":"<<r.backendGeneration
        <<",\"dispatchState\":\""<<escape(r.dispatchState)<<"\",\"verificationState\":\""<<escape(r.verificationState)
        <<"\",\"resultCategory\":\""<<escape(r.resultCategory)<<"\",\"errorCategory\":\""<<escape(r.errorCategory)
        <<"\",\"retryClassification\":\""<<escape(r.retryClassification)<<"\",\"boundedDiagnostics\":\""<<escape(r.boundedDiagnostics)
        <<"\",\"completedAt\":"<<r.completedAt<<'}';
    return out.str();
}