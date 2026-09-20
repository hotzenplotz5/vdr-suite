#ifndef VDR_SUITE_BRIDGE_HBBTV_PROVIDER_CONTRACT_H
#define VDR_SUITE_BRIDGE_HBBTV_PROVIDER_CONTRACT_H

#include <stdint.h>

// Provider ABI expected from hotzenplotz5/vdr-plugin-web.
// Discovery implementation: work/vdr-suite-hbbtv-discovery-v1
// Discovery anchor: 9ee1697a435e01058df6890323bf979a1ad2fd87
// Runtime implementation: work/vdr-suite-hbbtv-runtime-v1
// Runtime/presentation/media candidate: 1b3d2e785a1caf2d9f991fafa9752a433c5a86ae
// Upstream base: Zabrimus/vdr-plugin-web@34ded5090fbad021338c491355566dbdb4d98f9d
// Keep these contracts byte-compatible with the provider-side definitions.
#define VDRWEB_SERVICE_HBBTV_DISCOVERY_V1 "VdrWeb::HbbtvDiscovery-v1"
#define VDRWEB_SERVICE_HBBTV_RUNTIME_V1 "VdrWeb::HbbtvRuntime-v1"
#define VDRWEB_SERVICE_HBBTV_PRESENTATION_V1 "VdrWeb::HbbtvPresentation-v1"
#define VDRWEB_SERVICE_HBBTV_MEDIA_V1 "VdrWeb::HbbtvMedia-v1"

#define VDRWEB_HBBTV_SERVICE_SCHEMA_V1 1U
#define VDRWEB_HBBTV_CHANNEL_ID_MAX 64U
#define VDRWEB_HBBTV_APPLICATION_NAME_MAX 256U
#define VDRWEB_HBBTV_URL_BASE_MAX 1024U
#define VDRWEB_HBBTV_URL_LOCATION_MAX 512U
#define VDRWEB_HBBTV_URL_EXTENSION_MAX 512U
#define VDRWEB_HBBTV_MAX_APPLICATIONS 16U

#define VDRWEB_HBBTV_RUNTIME_SCHEMA_V1 1U
#define VDRWEB_HBBTV_SESSION_ID_MAX 128U
#define VDRWEB_HBBTV_PRESENTATION_SCHEMA_V1 1U
#define VDRWEB_HBBTV_PRESENTATION_CHUNK_MAX 49152U
#define VDRWEB_HBBTV_MEDIA_SCHEMA_V1 1U
#define VDRWEB_HBBTV_MEDIA_SOCKET_PATH_MAX 108U

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

enum VdrWebHbbtvRuntimeOperationV1 {
  VDRWEB_HBBTV_RUNTIME_LAUNCH = 1,
  VDRWEB_HBBTV_RUNTIME_STATUS = 2,
  VDRWEB_HBBTV_RUNTIME_INPUT = 3,
  VDRWEB_HBBTV_RUNTIME_CLOSE = 4
};

enum VdrWebHbbtvRuntimeResultV1 {
  VDRWEB_HBBTV_RUNTIME_RESULT_OK = 0,
  VDRWEB_HBBTV_RUNTIME_RESULT_ACCEPTED = 1,
  VDRWEB_HBBTV_RUNTIME_RESULT_INVALID_REQUEST = 2,
  VDRWEB_HBBTV_RUNTIME_RESULT_DISCOVERY_STALE = 3,
  VDRWEB_HBBTV_RUNTIME_RESULT_APPLICATION_NOT_LAUNCHABLE = 4,
  VDRWEB_HBBTV_RUNTIME_RESULT_BUSY = 5,
  VDRWEB_HBBTV_RUNTIME_RESULT_SESSION_NOT_ACTIVE = 6,
  VDRWEB_HBBTV_RUNTIME_RESULT_ACTION_UNSUPPORTED = 7,
  VDRWEB_HBBTV_RUNTIME_RESULT_RUNTIME_UNAVAILABLE = 8
};

enum VdrWebHbbtvRuntimeStateV1 {
  VDRWEB_HBBTV_RUNTIME_STATE_NONE = 0,
  VDRWEB_HBBTV_RUNTIME_STATE_STARTING = 1,
  VDRWEB_HBBTV_RUNTIME_STATE_ACTIVE = 2,
  VDRWEB_HBBTV_RUNTIME_STATE_CLOSING = 3,
  VDRWEB_HBBTV_RUNTIME_STATE_FAILED = 4
};

enum VdrWebHbbtvInputActionV1 {
  VDRWEB_HBBTV_INPUT_NONE = 0,
  VDRWEB_HBBTV_INPUT_UP = 1,
  VDRWEB_HBBTV_INPUT_DOWN = 2,
  VDRWEB_HBBTV_INPUT_LEFT = 3,
  VDRWEB_HBBTV_INPUT_RIGHT = 4,
  VDRWEB_HBBTV_INPUT_OK = 5,
  VDRWEB_HBBTV_INPUT_BACK = 6,
  VDRWEB_HBBTV_INPUT_RED = 7,
  VDRWEB_HBBTV_INPUT_GREEN = 8,
  VDRWEB_HBBTV_INPUT_YELLOW = 9,
  VDRWEB_HBBTV_INPUT_BLUE = 10,
  VDRWEB_HBBTV_INPUT_0 = 11,
  VDRWEB_HBBTV_INPUT_1 = 12,
  VDRWEB_HBBTV_INPUT_2 = 13,
  VDRWEB_HBBTV_INPUT_3 = 14,
  VDRWEB_HBBTV_INPUT_4 = 15,
  VDRWEB_HBBTV_INPUT_5 = 16,
  VDRWEB_HBBTV_INPUT_6 = 17,
  VDRWEB_HBBTV_INPUT_7 = 18,
  VDRWEB_HBBTV_INPUT_8 = 19,
  VDRWEB_HBBTV_INPUT_9 = 20,
  VDRWEB_HBBTV_INPUT_PLAY = 21,
  VDRWEB_HBBTV_INPUT_PAUSE = 22,
  VDRWEB_HBBTV_INPUT_STOP = 23,
  VDRWEB_HBBTV_INPUT_FAST_FORWARD = 24,
  VDRWEB_HBBTV_INPUT_REWIND = 25
};

struct VdrWebHbbtvRuntimeV1 {
  uint32_t structSize;

  // Request.
  uint8_t operation;
  uint8_t inputAction;
  uint16_t reservedRequest;
  uint32_t applicationId;
  uint64_t descriptorRevision;
  char sessionId[VDRWEB_HBBTV_SESSION_ID_MAX];
  char channelId[VDRWEB_HBBTV_CHANNEL_ID_MAX];

  // Response.
  uint32_t schemaVersion;
  uint8_t result;
  uint8_t state;
  uint16_t reservedResponse;
};

enum VdrWebHbbtvPresentationOperationV1 {
  VDRWEB_HBBTV_PRESENTATION_META = 1,
  VDRWEB_HBBTV_PRESENTATION_CHUNK = 2
};

enum VdrWebHbbtvPresentationResultV1 {
  VDRWEB_HBBTV_PRESENTATION_RESULT_OK = 0,
  VDRWEB_HBBTV_PRESENTATION_RESULT_INVALID_REQUEST = 1,
  VDRWEB_HBBTV_PRESENTATION_RESULT_NO_SESSION = 2,
  VDRWEB_HBBTV_PRESENTATION_RESULT_SESSION_MISMATCH = 3,
  VDRWEB_HBBTV_PRESENTATION_RESULT_NO_FRAME = 4,
  VDRWEB_HBBTV_PRESENTATION_RESULT_REVISION_MISMATCH = 5,
  VDRWEB_HBBTV_PRESENTATION_RESULT_RANGE_INVALID = 6,
  VDRWEB_HBBTV_PRESENTATION_RESULT_FRAME_TOO_LARGE = 7
};

enum VdrWebHbbtvPresentationVisibilityV1 {
  VDRWEB_HBBTV_PRESENTATION_VISIBILITY_UNKNOWN = 0,
  VDRWEB_HBBTV_PRESENTATION_VISIBILITY_HIDDEN = 1,
  VDRWEB_HBBTV_PRESENTATION_VISIBILITY_VISIBLE = 2
};

struct VdrWebHbbtvPresentationV1 {
  uint32_t structSize;

  // Request.
  uint8_t operation;
  uint8_t reservedRequest[3];
  uint64_t frameRevision;
  uint32_t offset;
  char sessionId[VDRWEB_HBBTV_SESSION_ID_MAX];

  // Response.
  uint32_t schemaVersion;
  uint8_t result;
  uint8_t visibility;
  uint8_t reservedResponse[2];
  uint64_t observedAt;
  uint32_t renderWidth;
  uint32_t renderHeight;
  uint32_t encodedBytes;
  uint32_t returnedBytes;
  uint8_t data[VDRWEB_HBBTV_PRESENTATION_CHUNK_MAX];
};

enum VdrWebHbbtvMediaResultV1 {
  VDRWEB_HBBTV_MEDIA_RESULT_OK = 0,
  VDRWEB_HBBTV_MEDIA_RESULT_INVALID_REQUEST = 1,
  VDRWEB_HBBTV_MEDIA_RESULT_NO_SESSION = 2,
  VDRWEB_HBBTV_MEDIA_RESULT_SESSION_MISMATCH = 3
};

enum VdrWebHbbtvMediaStateV1 {
  VDRWEB_HBBTV_MEDIA_STATE_NONE = 0,
  VDRWEB_HBBTV_MEDIA_STATE_STREAMING = 1,
  VDRWEB_HBBTV_MEDIA_STATE_PAUSED = 2,
  VDRWEB_HBBTV_MEDIA_STATE_STOPPED = 3,
  VDRWEB_HBBTV_MEDIA_STATE_FAILED = 4
};

struct VdrWebHbbtvMediaV1 {
  uint32_t structSize;
  char sessionId[VDRWEB_HBBTV_SESSION_ID_MAX];
  uint32_t schemaVersion;
  uint8_t result;
  uint8_t state;
  uint8_t fullscreen;
  uint8_t consumerConnected;
  uint64_t mediaRevision;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
  char socketPath[VDRWEB_HBBTV_MEDIA_SOCKET_PATH_MAX];
};

#endif
