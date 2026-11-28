#include "RestfulApiStatusMapper.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

#include <cassert>
#include <string>

static VdrConfig make_restfulapi_config()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = "127.0.0.1";
    config.port = 8002;
    return config;
}

static void test_status_mapper_maps_valid_info_response()
{
    VdrConfig config = make_restfulapi_config();

    VdrStatus status = RestfulApiStatusMapper::parseStatus(
        "{\"status\":\"ok\",\"version\":\"0.2.6\"}",
        config,
        200
    );

    assert(status.enabled == true);
    assert(status.mode == "restfulapi");
    assert(status.host == "127.0.0.1");
    assert(status.port == 8002);
    assert(status.state == "restfulapi");
}

static void test_status_mapper_reports_error_for_http_error()
{
    VdrConfig config = make_restfulapi_config();

    VdrStatus status = RestfulApiStatusMapper::parseStatus(
        "{\"status\":\"ok\"}",
        config,
        500
    );

    assert(status.enabled == true);
    assert(status.mode == "restfulapi");
    assert(status.host == "127.0.0.1");
    assert(status.port == 8002);
    assert(status.state == "error");
}

static void test_status_mapper_reports_error_for_invalid_json()
{
    VdrConfig config = make_restfulapi_config();

    VdrStatus status = RestfulApiStatusMapper::parseStatus(
        "not json",
        config,
        200
    );

    assert(status.state == "error");
}

static void test_status_mapper_accepts_empty_json_object()
{
    VdrConfig config = make_restfulapi_config();

    VdrStatus status = RestfulApiStatusMapper::parseStatus(
        "{}",
        config,
        200
    );

    assert(status.state == "restfulapi");
}

static void test_disk_capacity_uses_the_backend_info_object()
{
    const auto config = make_restfulapi_config();
    const auto parse = [&](const std::string& body, int code = 200) {
        return RestfulApiStatusMapper::parseStatus(body, config, code).diskFreeMb;
    };
    assert(parse(R"({"diskusage":{"free_mb":1572864,"used_percent":45}})") == 1572864);
    assert(parse(R"({"diskusage":{"free_mb":0}})") == 0);
    assert(parse(R"({"diskusage":{"free_mb":-1}})") == -1);
    assert(parse(R"({"diskusage":{"free_mb":1.5}})") == -1);
    assert(parse(R"({"diskusage":{"free_mb":"42"}})") == -1);
    assert(parse(R"({"diskusage":{"free_mb":9999999999999999999999}})") == -1);
    assert(parse(R"({"other":{"diskusage":{"free_mb":42}}})") == -1);
    assert(parse(R"({"diskusage":{"other":{"free_mb":42}}})") == -1);
    assert(parse(R"({"diskusage":{"free_mb":42}})", 500) == -1);
    assert(parse("{}") == -1);
}

int main()
{
    test_disk_capacity_uses_the_backend_info_object();
    test_status_mapper_maps_valid_info_response();
    test_status_mapper_reports_error_for_http_error();
    test_status_mapper_reports_error_for_invalid_json();
    test_status_mapper_accepts_empty_json_object();

    return 0;
}
