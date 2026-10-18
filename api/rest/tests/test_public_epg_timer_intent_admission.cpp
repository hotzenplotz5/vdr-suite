#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"
#include <cassert>
#include <string>
#include <ctime>

int main()
{
    auto& api = PublicApiRuntime::instance();
    const std::string route = "/api/v1/epg/timer-intents";
    const std::string body = "{\"backendId\":\"home\",\"channelId\":\"S19.2E-1-100\",\"eventId\":\"81234\"}";
    const std::string revision = "epg1:sha256:" + std::string(64, 'a');
    const std::string etag = vdrsuite::http::publicStrongEntityTag(revision);
    const auto post = [&](const std::string& input, const std::string& match,
        const std::string& key = "idem-epg-timer-1",
        const std::string& backend = "home",
        const std::string& target = "/api/v1/epg/timer-intents",
        const std::string& type = "application/json") {
        ApiResponse response;
        assert(api.tryHandlePost(target, "req", "corr", response,
            input, "actor", match, key, type, backend));
        return response;
    };
    api.resetEpgTimerIntentObservationLookup();
    api.resetEpgTimerIntentSubmit();
    api.resetEpgEventDetailLookup();
    assert(post(body, etag).statusCode == 503);
    int prematureResolverCalls = 0;
    api.registerEpgTimerIntentObservationLookup(
        [&](const PublicEpgTimerIntentAdmissionRequest&) {
            ++prematureResolverCalls;
            return PublicEpgTimerIntentObservationResult{};
        });
    assert(!api.epgTimerIntentAdmissionConfigured());
    assert(post(body, etag).statusCode == 503);
    assert(prematureResolverCalls == 0);
    api.resetEpgTimerIntentObservationLookup();
    assert(post(body, "").statusCode == 428);
    assert(post(body, "W/\"weak\"").statusCode == 400);
    assert(post(body, etag, "").statusCode == 400);
    assert(post(body, etag, "bad key").statusCode == 400);
    assert(post(body, etag, "idem", "foreign").statusCode == 400);
    assert(post(body, etag, "idem", "home", route + "?backendId=home").statusCode == 400);
    assert(post(body, etag, "idem", "home", route, "text/plain").statusCode == 415);
    assert(post("{", etag).statusCode == 400);
    assert(post("{}", etag).statusCode == 400);
    assert(post(body.substr(0,body.size()-1)+",\"title\":\"spoof\"}", etag).statusCode == 400);
    assert(post(body.substr(0,body.size()-1)+",\"eventId\":\"81234\"}", etag).statusCode == 400);
    assert(post(body,etag,"idem","home",route).headers.count("Location") == 0U);

    int observes=0, submits=0;
    PublicEpgTimerIntentObservationResult observed;
    observed.status = PublicEpgTimerIntentObservationStatus::ok;
    auto& o = observed.observation;
    o.backendId="home"; o.channelId="S19.2E-1-100";
    o.nativeEventId="81234"; o.sourceId="native-epg";
    o.resourceEtag=etag;
    o.startTime=4102444800LL; o.endTime=4102448400LL;
    o.backendGeneration=17;
    o.exactNativeIdentity=true; o.observationCurrent=true;
    observed.backendAllowsMutation=true;
    api.registerEpgEventDetailLookup([&](const PublicEpgEventDetailRequest& q) {
        PublicEpgEventDetailResult d;
        d.status=PublicEpgNowNextStatus::ok;
        d.backendId=q.backendId; d.channelId=q.channelId; d.eventId=q.eventId;
        d.resourceRevision=revision; return d;
    });
    api.registerEpgTimerIntentObservationLookup([&](const PublicEpgTimerIntentAdmissionRequest& r) {
        ++observes; assert(r.selection.eventId=="81234"); return observed;
    });
    auto status=PublicEpgTimerIntentAdmissionStatus::accepted;
    api.registerEpgTimerIntentSubmit([&](const PublicEpgTimerIntentAdmissionRequest& r,
        const vdrsuite::public_epg_timer::AuthoritativeObservation& current) {
        ++submits; assert(r.actorRef=="actor");
        assert(r.selection.ifMatch==etag); assert(r.selection.idempotencyKey=="idem-epg-timer-1");
        assert(current.backendGeneration==17);
        PublicEpgTimerIntentAdmissionResult v; v.status=status;
        v.operation.operationId="op_0123456789abcdef0123456789abcdef";
        v.operation.state="accepted"; v.operation.backendId="home";
        v.operation.resourceRevision="1"; return v;
    });
    assert(api.epgTimerIntentAdmissionConfigured());
    const auto accepted=post(body, etag);
    assert(accepted.statusCode==202 && submits==1);
    assert(accepted.headers.at("Location")=="/api/v1/operations/op_0123456789abcdef0123456789abcdef");
    assert(accepted.headers.at("Cache-Control")=="no-store");
    status=PublicEpgTimerIntentAdmissionStatus::replayed;
    assert(post(body,etag).statusCode==202);
    status=PublicEpgTimerIntentAdmissionStatus::idempotencyConflict;
    assert(post(body,etag).statusCode==409);
    status=PublicEpgTimerIntentAdmissionStatus::generationConflict;
    assert(post(body,etag).statusCode==409);
    status=PublicEpgTimerIntentAdmissionStatus::revisionConflict;
    assert(post(body,etag).statusCode==412);
    status=PublicEpgTimerIntentAdmissionStatus::serviceUnavailable;
    assert(post(body,etag).statusCode==503);
    const int called=submits;
    observed.backendAllowsMutation=false;
    assert(post(body,etag).statusCode==403 && submits==called);
    observed.backendAllowsMutation=true;
    observed.observation.resourceEtag=vdrsuite::http::publicStrongEntityTag("changed");
    assert(post(body,etag).statusCode==503 && submits==called);
    observed.observation.resourceEtag=etag;
    assert(post(body,vdrsuite::http::publicStrongEntityTag("other")).statusCode==412 && submits==called);
    observed.observation.backendGeneration=0;
    assert(post(body,etag).statusCode==503 && submits==called);
    observed.observation.backendGeneration=17;
    observed.observation.nativeEventId="wrong";
    assert(post(body,etag).statusCode==409 && submits==called);
    observed.observation.nativeEventId="81234";
    observed.observation.startTime=1700000000LL;
    observed.observation.endTime=1700003600LL;
    assert(post(body,etag).statusCode==409 && submits==called);
    api.resetEpgTimerIntentObservationLookup();
    api.resetEpgTimerIntentSubmit();
    api.resetEpgEventDetailLookup();
    assert(!api.epgTimerIntentAdmissionConfigured());
    ApiResponse read;
    assert(api.tryHandleGet(route,"actor","req","corr",read));
    assert(read.statusCode==405);
    return 0;
}
