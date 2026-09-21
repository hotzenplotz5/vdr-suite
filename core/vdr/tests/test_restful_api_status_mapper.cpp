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

int main()
{
    test_status_mapper_maps_valid_info_response();
    test_status_mapper_reports_error_for_http_error();
    test_status_mapper_reports_error_for_invalid_json();
    test_status_mapper_accepts_empty_json_object();

    return 0;
}
