RECORDING_ACTION_CORE_SRC := \
        core/recordings/src/RecordingActionUtils.cpp \
        core/recordings/src/RecordingActionValidationResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionExecutionResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionValidationService.cpp

RECORDING_ACTION_REST_PARSER_SRC := \
        api/rest/src/RecordingActionValidationRequestParser.cpp

RECORDING_ACTION_REST_CONTROLLER_SRC := \
        $(RECORDING_ACTION_CORE_SRC) \
        $(RECORDING_ACTION_REST_PARSER_SRC)

RECORDING_ACTION_VDR_CACHE_SRC := \
        core/vdr/src/SearchTimerPreviewEpgCache.cpp

RECORDING_ACTION_PREVIEW_SRC := \
        $(RECORDING_ACTION_CORE_SRC)

RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC := \
        $(RECORDING_ACTION_CORE_SRC) \
        core/recordings/src/RestfulApiRecordingActionExecutor.cpp

RECORDING_ACTION_EXECUTOR_ADAPTER_SRC := \
        $(RECORDING_ACTION_CORE_SRC)

RECORDING_ACTION_LEGACY_TEST_SRC := \
        $(RECORDING_ACTION_CORE_SRC)

ACTION_SERVICE_SRC := \
        core/recordings/src/ActionService.cpp \
        core/recordings/src/RecordingActionUtils.cpp

JOB_SERVICE_SRC := \
        core/recordings/src/JobService.cpp \
        core/recordings/src/RecordingActionUtils.cpp

JOB_REPOSITORY_SRC := \
        core/recordings/src/JobRepository.cpp \
        core/recordings/src/JobService.cpp \
        core/recordings/src/RecordingActionUtils.cpp

WORKFLOW_SRC := \
        core/recordings/src/RecordingWorkflowService.cpp \
        core/recordings/src/ActionService.cpp \
        core/recordings/src/JobService.cpp \
        core/recordings/src/JobRepository.cpp \
        core/recordings/src/RecordingActionUtils.cpp

WORKER_SRC := \
        core/recordings/src/WorkerSimulator.cpp

RECTOOLS_ADAPTER_SRC := \
        core/recordings/src/RectoolsAdapter.cpp \
        core/recordings/src/JobService.cpp \
        core/recordings/src/RecordingActionUtils.cpp
