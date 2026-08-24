#include "RestfulApiRecordingMapper.h"
#include "VdrRecording.h"

#include <cassert>
#include <string>
#include <vector>

static void test_parse_recordings_empty_array()
{
    std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings("{\"recordings\":[]}");

    assert(recordings.empty() == true);
}

static void test_parse_recordings_maps_real_restfulapi_shape()
{
    const std::string json =
        "{\"recordings\":["
        "{\"number\":0,"
        "\"name\":\"Mystery~The Village - Das Dorf\","
        "\"file_name\":\"/srv/vdr/video/Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec\","
        "\"relative_file_name\":\"/Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec\","
        "\"inode\":\"2065:42467975\","
        "\"is_new\":true,"
        "\"is_edited\":false,"
        "\"is_pes_recording\":false,"
        "\"duration\":5835,"
        "\"filesize_mb\":5555,"
        "\"channel_id\":\"C-1-1107-898\","
        "\"frames_per_second\":25,"
        "\"marks\":[],"
        "\"event_title\":\"The Village - Das Dorf\","
        "\"event_short_text\":\"\","
        "\"event_start_time\":1288488540,"
        "\"event_duration\":5820"
        "},"
        "{\"number\":840,"
        "\"name\":\"Serien~The Walking Dead~S08E08 Kampf um die Zukunft\","
        "\"file_name\":\"/srv/vdr/video/Serien/The_Walking_Dead/S08E08_Kampf_um_die_Zukunft/2017-12-11.21.01.4-0.rec\","
        "\"relative_file_name\":\"/Serien/The_Walking_Dead/S08E08_Kampf_um_die_Zukunft/2017-12-11.21.01.4-0.rec\","
        "\"duration\":3551,"
        "\"filesize_mb\":4191,"
        "\"event_start_time\":1513022400,"
        "\"event_duration\":3900"
        "}"
        "]}";

    std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 2);

    assert(recordings[0].id == "0");
    assert(recordings[0].title == "Mystery/The Village - Das Dorf");
    assert(recordings[0].path == "/Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec");
    assert(recordings[0].backendNativeId == "/srv/vdr/video/Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec");
    assert(recordings[0].startTime == "1288488540");
    assert(recordings[0].durationSeconds == 5835);
    assert(recordings[0].recordingDurationKnown);
    assert(recordings[0].sizeMb == 5555);

    assert(recordings[1].id == "840");
    assert(recordings[1].title == "Serien/The Walking Dead/S08E08 Kampf um die Zukunft");
    assert(recordings[1].path == "/Serien/The_Walking_Dead/S08E08_Kampf_um_die_Zukunft/2017-12-11.21.01.4-0.rec");
    assert(recordings[1].backendNativeId == "/srv/vdr/video/Serien/The_Walking_Dead/S08E08_Kampf_um_die_Zukunft/2017-12-11.21.01.4-0.rec");
    assert(recordings[1].startTime == "1513022400");
    assert(recordings[1].durationSeconds == 3551);
    assert(recordings[1].recordingDurationKnown);
    assert(recordings[1].sizeMb == 4191);
}

static void test_parse_recordings_marks_event_duration_as_non_authoritative()
{
    const std::string json =
        "{\"recordings\":["
        "{\"number\":9,"
        "\"name\":\"Event duration fallback\","
        "\"file_name\":\"/srv/vdr/video/Fallback/2026-06-01.20.00.1-0.rec\","
        "\"duration\":-1,"
        "\"event_duration\":3600,"
        "\"filesize_mb\":42,"
        "\"event_start_time\":1780000000}"
        "]}";

    const std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 1);
    assert(recordings[0].durationSeconds == 3600);
    assert(!recordings[0].recordingDurationKnown);
}

static void test_parse_recordings_falls_back_to_absolute_file_name()
{
    const std::string json =
        "{\"recordings\":["
        "{\"number\":7,"
        "\"name\":\"Fallback Recording\","
        "\"file_name\":\"/srv/vdr/video/Fallback/2026-06-01.20.00.1-0.rec\","
        "\"duration\":60,"
        "\"filesize_mb\":42,"
        "\"event_start_time\":1780000000"
        "}"
        "]}";

    std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 1);
    assert(recordings[0].id == "7");
    assert(recordings[0].path == "/srv/vdr/video/Fallback/2026-06-01.20.00.1-0.rec");
    assert(recordings[0].backendNativeId == "/srv/vdr/video/Fallback/2026-06-01.20.00.1-0.rec");
}

static void test_parse_recordings_ignores_objects_without_number()
{
    const std::string json =
        "{\"recordings\":["
        "{\"name\":\"Broken Recording\","
        "\"relative_file_name\":\"/Broken/2026-06-01.20.00.1-0.rec\","
        "\"duration\":60,"
        "\"filesize_mb\":42,"
        "\"event_start_time\":1780000000"
        "}"
        "]}";

    std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.empty() == true);
}


static void test_parse_recordings_imports_additional_media_actors()
{
    const std::string json =
        "{\"recordings\":["
        "{\"number\":1,"
        "\"name\":\"Movies~Forrest Gump\","
        "\"file_name\":\"/srv/vdr/video/Movies/Forrest_Gump/2026-01-01.20.15.1-0.rec\","
        "\"duration\":8000,"
        "\"filesize_mb\":7000,"
        "\"event_start_time\":1780000000,"
        "\"additional_media\":{"
        "\"scraper\":\"movie\","
        "\"movie_id\":13,"
        "\"movie_title\":\"Forrest Gump\","
        "\"actors\":["
        "{\"name\":\"Tom Hanks\",\"role\":\"Forrest Gump\",\"thumb\":\"tom.jpg\"},"
        "{\"name\":\"Robin Wright\",\"role\":\"Jenny Curran\",\"thumb\":\"robin.jpg\"}"
        "]"
        "}"
        "}"
        "]}";

    std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 1);
    assert(recordings[0].persons.size() == 2);
    assert(recordings[0].persons.hasRole(PersonRole::Actor));
    assert(recordings[0].persons.hasNormalizedName("tom-hanks"));
    assert(recordings[0].persons.hasNormalizedName("robin-wright"));

    const Person& firstActor = recordings[0].persons.all().at(0);
    assert(firstActor.source() == ContentClassificationSource::Tvscraper);
    assert(firstActor.role() == PersonRole::Actor);
    assert(firstActor.originalName() == "Tom Hanks");
    assert(firstActor.normalizedName() == "tom-hanks");
    assert(firstActor.characterName() == "Forrest Gump");
}

static void test_parse_recordings_derives_start_time_from_rec_directory()
{
    const std::string json =
        "{\"recordings\":["
        "{\"number\":226,"
        "\"name\":\"Action~48 Hrs\","
        "\"file_name\":\"/srv/vdr/video/Action/48 Hrs/2026-06-21.10.08.1-0.rec\","
        "\"relative_file_name\":\"/Action/48 Hrs/2026-06-21.10.08.1-0.rec\","
        "\"duration\":-1,"
        "\"event_duration\":-1,"
        "\"event_start_time\":-1,"
        "\"filesize_mb\":9232"
        "}"
        "]}";

    std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 1);
    assert(recordings[0].id == "226");
    assert(recordings[0].path == "/Action/48 Hrs/2026-06-21.10.08.1-0.rec");
    assert(recordings[0].durationSeconds == -1);
    assert(!recordings[0].recordingDurationKnown);
    assert(recordings[0].startTime != "");
    assert(recordings[0].startTime != "-1");
    assert(std::stoll(recordings[0].startTime) > 1000000000);
}

static void test_parse_recordings_prefers_canonical_entry_over_recursive_aliases()
{
    const std::string json =
        "{\"recordings\":["
        "{\"number\":3652,"
        "\"name\":\"Recordings on yavdr(nfs)~Recordings on yavdr(nfs)~heute journal\","
        "\"file_name\":\"/srv/vdr/video/Recordings_on_yavdr(nfs)/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\","
        "\"relative_file_name\":\"/Recordings_on_yavdr(nfs)/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\","
        "\"inode\":\"\","
        "\"duration\":341,"
        "\"filesize_mb\":284,"
        "\"event_start_time\":1783539900},"
        "{\"number\":7378,"
        "\"name\":\"Recordings on yavdr(nfs)~heute journal\","
        "\"file_name\":\"/srv/vdr/video/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\","
        "\"relative_file_name\":\"/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\","
        "\"inode\":\"\","
        "\"duration\":341,"
        "\"filesize_mb\":284,"
        "\"event_start_time\":1783539900},"
        "{\"number\":7999,"
        "\"name\":\"VDR-SUITE-TEST heute journal\","
        "\"file_name\":\"/srv/vdr/video/VDR-SUITE-TEST_heute_journal/2026-07-08.21.45.2-0.rec\","
        "\"relative_file_name\":\"/VDR-SUITE-TEST_heute_journal/2026-07-08.21.45.2-0.rec\","
        "\"inode\":\"2065:14947898\","
        "\"duration\":341,"
        "\"filesize_mb\":284,"
        "\"event_start_time\":1783539900}"
        "]}";

    const std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 1);
    assert(recordings[0].id == "7999");
    assert(recordings[0].title == "VDR-SUITE-TEST heute journal");
    assert(recordings[0].backendNativeId ==
        "/srv/vdr/video/VDR-SUITE-TEST_heute_journal/2026-07-08.21.45.2-0.rec");
}

static void test_parse_recordings_preserves_distinct_real_copies()
{
    const std::string json =
        "{\"recordings\":["
        "{\"number\":10,"
        "\"name\":\"Copy A\","
        "\"file_name\":\"/srv/vdr/video/Copy_A/2026-07-08.21.45.2-0.rec\","
        "\"inode\":\"2065:100\","
        "\"duration\":341,"
        "\"filesize_mb\":284,"
        "\"event_start_time\":1783539900},"
        "{\"number\":11,"
        "\"name\":\"Copy B\","
        "\"file_name\":\"/srv/vdr/video/Copy_B/2026-07-08.21.45.2-0.rec\","
        "\"inode\":\"2065:101\","
        "\"duration\":341,"
        "\"filesize_mb\":284,"
        "\"event_start_time\":1783539900}"
        "]}";

    const std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 2);
}

static void test_parse_recordings_tolerates_invalid_json()
{
    std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings("not json");

    assert(recordings.empty() == true);
}

int main()
{
    test_parse_recordings_empty_array();
    test_parse_recordings_maps_real_restfulapi_shape();
    test_parse_recordings_marks_event_duration_as_non_authoritative();
    test_parse_recordings_falls_back_to_absolute_file_name();
    test_parse_recordings_ignores_objects_without_number();
    test_parse_recordings_imports_additional_media_actors();
    test_parse_recordings_derives_start_time_from_rec_directory();
    test_parse_recordings_prefers_canonical_entry_over_recursive_aliases();
    test_parse_recordings_preserves_distinct_real_copies();
    test_parse_recordings_tolerates_invalid_json();

    return 0;
}