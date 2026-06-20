#include "PersonController.h"
#include "PersonResolutionJsonSerializer.h"
#include "PersonResolver.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    PersonResolutionJsonSerializer serializer;
    PersonController controller(serializer);

    ApiResponse unresolvedResponse =
        controller.getPersonResolution(
            PersonResolutionResult::unresolved());

    assert(unresolvedResponse.statusCode == 200);
    assert(unresolvedResponse.contentType == "application/json");
    assert(unresolvedResponse.body.find("\"resolved\":false") != std::string::npos);
    assert(unresolvedResponse.body.find("\"primaryPerson\":null") != std::string::npos);

    PersonCollection persons = PersonCollection::createEmpty();

    persons.add(
        Person::withProviderReference(
            ContentClassificationSource::Tmdb,
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump",
            90,
            "tmdb:31"));

    PersonResolver resolver;

    ApiResponse resolvedResponse =
        controller.getPersonResolution(
            resolver.resolve(persons));

    assert(resolvedResponse.statusCode == 200);
    assert(resolvedResponse.contentType == "application/json");
    assert(resolvedResponse.body.find("\"resolved\":true") != std::string::npos);
    assert(resolvedResponse.body.find("\"source\":\"tmdb\"") != std::string::npos);
    assert(resolvedResponse.body.find("\"role\":\"actor\"") != std::string::npos);
    assert(resolvedResponse.body.find("\"normalizedName\":\"tom-hanks\"") != std::string::npos);

    std::cout << "test_person_controller passed" << std::endl;
    return 0;
}
