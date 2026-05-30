#include "RecordingService.h"
#include "RecordingRepository.h"

RecordingService::RecordingService(RecordingRepository& repository)
    : repository_(repository)
{
}

std::vector<Recording> RecordingService::getAllRecordings()
{
    return repository_.getAllRecordings();
}
