#include "ContentRatingController.h"
#include "ContentRatingResolutionJsonSerializer.h"
#include "ContentRatingResolver.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    ContentRatingResolutionJsonSerializer serializer;
    ContentRatingController controller(serializer);

    ApiResponse unresolvedResponse =
        controller.getRatingResolution(
            ContentRatingResolutionResult::unresolved());

    assert(unresolvedResponse.statusCode == 200);
    assert(unresolvedResponse.contentType == "application/json");
    assert(unresolvedResponse.body.find("\"resolved\":false") != std::string::npos);
    assert(unresolvedResponse.body.find("\"primaryRating\":null") != std::string::npos);

    ContentRatingCollection ratings = ContentRatingCollection::createEmpty();

    ratings.add(
        ContentRating::withConfidence(
            ContentClassificationSource::Tmdb,
            ContentRatingSystem::ProviderSpecific,
            "DE:16",
            16,
            90));

    ContentRatingResolver resolver;

    ApiResponse resolvedResponse =
        controller.getRatingResolution(
            resolver.resolve(ratings));

    assert(resolvedResponse.statusCode == 200);
    assert(resolvedResponse.contentType == "application/json");
    assert(resolvedResponse.body.find("\"resolved\":true") != std::string::npos);
    assert(resolvedResponse.body.find("\"minimumAge\":16") != std::string::npos);
    assert(resolvedResponse.body.find("\"source\":\"tmdb\"") != std::string::npos);

    std::cout << "test_content_rating_controller passed" << std::endl;
    return 0;
}
