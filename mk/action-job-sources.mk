ACTIONS_SRC := \
        core/recordings/src/RecordingActionUtils.cpp \
        core/recordings/src/RecordingActionValidationResultJsonSerializer.cpp

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
