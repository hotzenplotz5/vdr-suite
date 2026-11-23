#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"
#include <cassert>
#include <string>
#include <ctime>


static void test_manual_timer_intent_post()
{
    auto& api = PublicApiRuntime::instance();
    const std::string route = "/api/v1/manual/timer-intents";
    const std::string body = R"json({"backendId":"home","channelId":"S19.2E-1-100","title":"News, Teil 1","directory":"News","startAt":1795547700,"stopAt":1795552200,"priority":0,"lifetime":0,"enabled":true,"vps":false})json";
    const auto post = [&](const std::string& input,
        const std::string& key = "manual-idem-1",
        const std::string& backend = "home",
        const std::string& target = "/api/v1/manual/timer-intents",
        const std::string& type = "application/json") {
        ApiResponse response;
        assert(api.tryHandlePost(target, "req", "corr", response,
            input, "actor", "", key, type, backend));
        return response;
    };
    api.resetManualTimerIntentReplayLookup();
    api.resetManualTimerIntentSubmit();
    assert(post(body).statusCode == 503);
    assert(post(body, "").statusCode == 400);
    assert(post(body, "bad key").statusCode == 400);
    assert(post(body, "manual-idem-1", "foreign").statusCode == 400);
    assert(post(body, "manual-idem-1", "home", route + "?backendId=home").statusCode == 400);
    assert(post(body, "manual-idem-1", "home", route, "text/plain").statusCode == 415);
    assert(post("{").statusCode == 400);
    assert(post("{}").statusCode == 400);
    assert(post(body.substr(0, body.size()-1) +
        ",\"eventId\":\"unverified\"}").statusCode == 400);
    assert(post(body.substr(0, body.size()-1) +
        ",\"title\":\"duplicate\"}").statusCode == 400);
    assert(post(body.substr(0, body.size()-1) +
        ",\"priority\":200}").statusCode == 400);
    assert(post(body.substr(0, body.size()-1) +
        ",\"aux\":\"<epgsearch>spoof</epgsearch>\"}").statusCode == 400);

    int replays = 0, submissions = 0;
    auto replayStatus = PublicEpgTimerIntentAdmissionStatus::notFound;
    auto submitStatus = PublicEpgTimerIntentAdmissionStatus::accepted;
    api.registerManualTimerIntentReplayLookup(
        [&](const PublicManualTimerIntentAdmissionRequest& r) {
            ++replays;
            assert(r.actorRef=="actor" && r.backendId=="home");
            assert(r.channelId=="S19.2E-1-100");
            assert(r.title=="News, Teil 1" && r.directory=="News");
            assert(r.priority==0 && r.lifetime==0);
            assert(r.startAt==1795547700LL && r.stopAt==1795552200LL);
            assert(r.enabled && !r.vps && r.idempotencyKey=="manual-idem-1");
            PublicEpgTimerIntentAdmissionResult result;
            result.status=replayStatus;
            result.operation.operationId="op_0123456789abcdef0123456789abcdef";
            result.operation.state="outcome_unknown";
            result.operation.backendId="home";
            result.operation.resourceRevision="2";
            return result;
        });
    api.registerManualTimerIntentSubmit(
        [&](const PublicManualTimerIntentAdmissionRequest& r) {
            ++submissions;
            assert(r.actorRef=="actor" && r.idempotencyKey=="manual-idem-1");
            PublicEpgTimerIntentAdmissionResult result;
            result.status=submitStatus;
            result.operation.operationId="op_0123456789abcdef0123456789abcdef";
            result.operation.state="accepted";
            result.operation.backendId="home";
            result.operation.resourceRevision="1";
            return result;
        });
    const auto accepted = post(body);
    assert(accepted.statusCode==202 && submissions==1 && replays==1);
    assert(accepted.headers.at("Location") ==
        "/api/v1/operations/op_0123456789abcdef0123456789abcdef");
    assert(accepted.headers.at("Cache-Control")=="no-store");
    // Public-v1 returns the flat DurableOperation resource.
    // HTTP 202 acknowledges a durable Operation, not native Timer success.
    assert(accepted.body.find(
        R"json("operationId":"op_0123456789abcdef0123456789abcdef")json") !=
        std::string::npos);
    assert(accepted.body.find(R"json("state":"accepted")json") !=
        std::string::npos);
    assert(accepted.body.find(R"json("backendId":"home")json") !=
        std::string::npos);
    assert(accepted.body.find(
        R"json("links":{"self":"/api/v1/operations/op_0123456789abcdef0123456789abcdef"})json") !=
        std::string::npos);
    assert(accepted.body.find(R"json("operation":)json") ==
        std::string::npos);
    replayStatus = PublicEpgTimerIntentAdmissionStatus::replayed;
    api.resetManualTimerIntentSubmit();
    const auto recovered = post(body);
    assert(recovered.statusCode==202);
    assert(recovered.body.find("outcome_unknown") != std::string::npos);
    assert(submissions==1 && replays==2);
    replayStatus = PublicEpgTimerIntentAdmissionStatus::idempotencyConflict;
    assert(post(body).statusCode==409 && submissions==1);
    replayStatus = PublicEpgTimerIntentAdmissionStatus::notFound;
    assert(post(body).statusCode==503 && submissions==1);
    api.registerManualTimerIntentSubmit(
        [&](const PublicManualTimerIntentAdmissionRequest&) {
            ++submissions;
            PublicEpgTimerIntentAdmissionResult result;
            result.status=submitStatus;
            return result;
        });
    submitStatus = PublicEpgTimerIntentAdmissionStatus::stateConflict;
    assert(post(body).statusCode==409);
    submitStatus = PublicEpgTimerIntentAdmissionStatus::backendUnavailable;
    assert(post(body).statusCode==503);
    submitStatus = PublicEpgTimerIntentAdmissionStatus::notFound;
    assert(post(body).statusCode==404);
    // Valid JSON Unicode escapes must survive the public manual CREATE
    // adapter exactly as the existing Web Timer Editor transports titles.
    // This also proves that an escaped key still cannot bypass the closed
    // ten-field contract and that replay is not confused with a new write.
    int unicodeReplayCalls = 0, unicodeSubmitCalls = 0;
    const std::string escaped = R"json({"backendId":"home","channelId":"S19.2E-1-100","title":"K\u00f6ln, Spezial","directory":"Serien\u002fArchiv","startAt":1795547700,"stopAt":1795552200,"priority":0,"lifetime":0,"enabled":false,"vps":true})json";
    api.registerManualTimerIntentReplayLookup(
        [&](const PublicManualTimerIntentAdmissionRequest& r) {
            ++unicodeReplayCalls;
            assert(r.title == "Köln, Spezial");
            assert(r.directory == "Serien/Archiv");
            assert(r.priority == 0 && r.lifetime == 0);
            assert(!r.enabled && r.vps);
            PublicEpgTimerIntentAdmissionResult result;
            result.status = PublicEpgTimerIntentAdmissionStatus::notFound;
            return result;
        });
    api.registerManualTimerIntentSubmit(
        [&](const PublicManualTimerIntentAdmissionRequest& r) {
            ++unicodeSubmitCalls;
            assert(r.title == "Köln, Spezial");
            PublicEpgTimerIntentAdmissionResult result;
            result.status = PublicEpgTimerIntentAdmissionStatus::accepted;
            result.operation.operationId = "op_0123456789abcdef0123456789abcdef";
            result.operation.state = "accepted";
            result.operation.backendId = "home";
            result.operation.resourceRevision = "1";
            return result;
        });
    assert(post(escaped).statusCode == 202);
    assert(unicodeReplayCalls == 1 && unicodeSubmitCalls == 1);
    // Malformed Unicode and duplicate decoded keys must never dispatch.
    std::string invalidUnicode = escaped;
    const auto unicodeAt = invalidUnicode.find(R"json(\u00f6)json");
    assert(unicodeAt != std::string::npos);
    invalidUnicode.replace(unicodeAt, 6, R"json(\u00g6)json");
    assert(post(invalidUnicode).statusCode == 400);
    const auto duplicateKey = escaped.substr(0, escaped.size() - 1) +
        R"json(,"\u0074itle":"duplicate"})json";
    assert(post(duplicateKey).statusCode == 400);
    assert(unicodeReplayCalls == 1 && unicodeSubmitCalls == 1);
    api.resetManualTimerIntentReplayLookup();
    api.resetManualTimerIntentSubmit();
    ApiResponse get;
    assert(api.tryHandleGet(route, "actor", "req", "corr", get));
    assert(get.statusCode==405);
}

int main()
{
    test_manual_timer_intent_post();
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
    api.resetEpgTimerIntentReplayLookup();
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
    int replayQueries=0;
    auto replayStatus=PublicEpgTimerIntentAdmissionStatus::notFound;
    api.registerEpgTimerIntentReplayLookup([&](const PublicEpgTimerIntentAdmissionRequest& r) {
        ++replayQueries;
        assert(r.actorRef=="actor" && r.selection.backendId=="home");
        PublicEpgTimerIntentAdmissionResult v; v.status=replayStatus;
        v.operation.operationId="op_0123456789abcdef0123456789abcdef";
        v.operation.state="outcome_unknown"; v.operation.backendId="home";
        v.operation.resourceRevision="2"; return v;
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
    const int oldObservations=observes, oldSubmits=submits;
    replayStatus=PublicEpgTimerIntentAdmissionStatus::replayed;
    observed.observation.startTime=1; observed.observation.endTime=100;
    const auto recovered=post(body,etag);
    assert(recovered.statusCode==202);
    assert(recovered.headers.at("Location")==
        "/api/v1/operations/op_0123456789abcdef0123456789abcdef");
    assert(observes==oldObservations && submits==oldSubmits);
    // A committed Operation is still queryable even while fresh CREATE
    // remains intentionally unregistered (no live-time evidence assumed).
    api.resetEpgTimerIntentSubmit();
    assert(post(body,etag).statusCode==202);
    replayStatus=PublicEpgTimerIntentAdmissionStatus::idempotencyConflict;
    assert(post(body,etag).statusCode==409);
    assert(observes==oldObservations && submits==oldSubmits);
    replayStatus=PublicEpgTimerIntentAdmissionStatus::notFound;
    assert(post(body,etag).statusCode==503);
    api.registerEpgTimerIntentSubmit([&](const PublicEpgTimerIntentAdmissionRequest& r,
        const vdrsuite::public_epg_timer::AuthoritativeObservation& current) {
        ++submits; assert(r.actorRef=="actor"); assert(current.backendGeneration==17);
        PublicEpgTimerIntentAdmissionResult v; v.status=status;
        v.operation.operationId="op_0123456789abcdef0123456789abcdef";
        v.operation.state="accepted"; v.operation.backendId="home";
        v.operation.resourceRevision="1"; return v;
    });
    replayStatus=PublicEpgTimerIntentAdmissionStatus::notFound;
    observed.observation.startTime=4102444800LL;
    observed.observation.endTime=4102448400LL;
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
    api.resetEpgTimerIntentReplayLookup();
    api.resetEpgTimerIntentSubmit();
    api.resetEpgEventDetailLookup();
    assert(!api.epgTimerIntentAdmissionConfigured());
    ApiResponse read;
    assert(api.tryHandleGet(route,"actor","req","corr",read));
    assert(read.statusCode==405);
    return 0;
}
