#ifndef VDR_SUITE_BRIDGE_HBBTV_PROVIDER_CONTRACT_H
#define VDR_SUITE_BRIDGE_HBBTV_PROVIDER_CONTRACT_H

#include <stdint.h>

// Provider ABI expected from hotzenplotz5/vdr-plugin-web.
// Discovery implementation: work/vdr-suite-hbbtv-discovery-v1
// pinned commit: 9ee1697a435e01058df6890323bf979a1ad2fd87
// upstream base: Zabrimus/vdr-plugin-web@34ded5090fbad021338c491355566dbdb4d98f9d
// Keep this contract byte-compatible with the provider-side definition.
#define VDRWEB_SERVICE_HBBTV_DISCOVERY_V1 "VdrWeb::HbbtvDiscovery-v1"

#define VDRWEB_HBBTV_SERVICE_SCHEMA_V1 1U
#define VDRWEB_HBBTV_CHANNEL_ID_MAX 64U
#define VDRWEB_HBBTV_APPLICATION_NAME_MAX 256U
#define VDRWEB_HBBTV_URL_BASE_MAX 1024U
#define VDRWEB_HBBTV_URL_LOCATION_MAX 512U
#define VDRWEB_HBBTV_URL_EXTENSION_MAX 512U
#define VDRWEB_HBBTV_MAX_APPLICATIONS 16U

enum VdrWebHbbtvDiscoveryResultV1 {
  VDRWEB_HBBTV_RESULT_OK = 0,
  VDRWEB_HBBTV_RESULT_INVALID_REQUEST = 1,
  VDRWEB_HBBTV_RESULT_NO_LIVE_SERVICE = 2,
  VDRWEB_HBBTV_RESULT_CHANNEL_MISMATCH = 3,
  VDRWEB_HBBTV_RESULT_NO_APPLICATIONS = 4,
  VDRWEB_HBBTV_RESULT_RECEIVER_INACTIVE = 5
};

struct VdrWebHbbtvApplicationV1 {
  uint32_t applicationId;
  uint8_t controlCode;
  uint8_t priority;
  uint8_t reserved[2];
  char name[VDRWEB_HBBTV_APPLICATION_NAME_MAX];
  char urlBase[VDRWEB_HBBTV_URL_BASE_MAX];
  char urlLocation[VDRWEB_HBBTV_URL_LOCATION_MAX];
  char urlExtension[VDRWEB_HBBTV_URL_EXTENSION_MAX];
};

struct VdrWebHbbtvDiscoveryV1 {
  uint32_t structSize;

  // Request: the Suite/VDR channel identity expected to be current.
  char channelId[VDRWEB_HBBTV_CHANNEL_ID_MAX];

  // Response.
  uint32_t schemaVersion;
  uint8_t result;
  uint8_t receiverActive;
  uint16_t applicationCount;
  uint64_t discoveryRevision;
  uint64_t observedAt;
  VdrWebHbbtvApplicationV1 applications[VDRWEB_HBBTV_MAX_APPLICATIONS];
};

#endif
