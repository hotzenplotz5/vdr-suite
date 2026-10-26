#include "AddonActivationHttpPayload.h"
#include <cassert>
#include <string>
int main() {
    using namespace vdrsuite::addons;
    ActivationHttpMutation request;
    assert(parseActivationHttpMutation(
        R"({"backendId":"backend:a","desiredEnabled":true,"expectedRevision":0})",request));
    assert(request.backendId=="backend:a" && request.enabled && request.expectedRevision==0);
    assert(parseActivationHttpMutation(
        R"( { "backendId" : "backend-b", "desiredEnabled" : false, "expectedRevision" : 143 } )",request));
    assert(!request.enabled && request.expectedRevision==143);
    std::string backend;
    assert(parseActivationHttpBackendQuery(
        "/api/v1/addons/rectools/activation?backendId=backend:a", backend));
    assert(backend=="backend:a");
    assert(!parseActivationHttpBackendQuery(
        "/api/v1/addons/rectools/activation?backendId=../etc", backend));
    assert(!parseActivationHttpBackendQuery(
        "/api/v1/addons/rectools/activation?backendId=backend-a&backendId=other", backend));
    for(const char* invalid : {
        R"({"backendId":"../etc","desiredEnabled":true,"expectedRevision":0})",
        R"({"backendId":"*","desiredEnabled":true,"expectedRevision":0})",
        R"({"backendId":"backend","desiredEnabled":"true","expectedRevision":0})",
        R"({"backendId":"backend","desiredEnabled":true,"expectedRevision":-1})",
        R"({"backendId":"backend","desiredEnabled":true,"expectedRevision":0,"packageTrusted":true})",
        R"({"backendId":"backend","desiredEnabled":true,"expectedRevision":0,"backendId":"other"})",
        R"({"backendId":"backend","desiredEnabled":true,"expectedRevision":99999999999999999999})",
        R"({"backendId":"backend","desiredEnabled":null,"expectedRevision":0})"
    }) assert(!parseActivationHttpMutation(invalid,request));
    return 0;
}
