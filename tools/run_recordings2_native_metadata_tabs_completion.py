#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PATCH_PATH = ROOT / "tools/apply_recordings2_native_metadata_tabs_completion.py"

spec = importlib.util.spec_from_file_location(
    "recordings2_native_metadata_completion",
    PATCH_PATH,
)
if spec is None or spec.loader is None:
    raise SystemExit("Completion-Patch konnte nicht geladen werden")

patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)

original_create_metadata_detail_module = patch.create_metadata_detail_module


def create_metadata_detail_module(editor):
    original_create_metadata_detail_module(editor)

    editor.replace_once(
        "web/frontend/recordings2-metadata-detail.js",
        '''    if (header && header.nextSibling) root.insertBefore(tabs, header.nextSibling);
    else root.insertBefore(tabs, root.firstChild || null);

    const loading = status('Persistierte SuiteBridge-/TVScraper-Metadaten werden geladen …', false);
    root.insertBefore(loading, panels.recording);
    root.appendChild(panels.recording);
''',
        '''    if (header) {
      if (header.nextSibling) root.insertBefore(tabs, header.nextSibling);
      else root.appendChild(tabs);
    } else {
      root.insertBefore(tabs, root.firstChild || null);
    }

    const loading = status('Persistierte SuiteBridge-/TVScraper-Metadaten werden geladen …', false);
    root.appendChild(panels.recording);
    root.insertBefore(loading, panels.recording);
''',
    )


def create_serializer_test(editor):
    content = r'''#include "VdrRecordingNativeMetadataPublicJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    VdrRecordingNativeMetadataRecord record;
    record.backendId = "default";
    record.backendNativeId =
        "/srv/vdr/video/Filme/Inferno/2026-07-20.20.15.1-0.rec";
    record.recordingKey = "recording-key";
    record.contentState = "found";

    record.metadata.found = true;
    record.metadata.provider = "tvscraper";
    record.metadata.mediaType = "movie";
    record.metadata.providerId = 207932;
    record.metadata.title = "Inferno";
    record.metadata.overview =
        "Robert Langdon folgt einer gefährlichen Spur.";
    record.metadata.releaseDate = "2016-10-13";
    record.metadata.imdbId = "tt3062096";
    record.metadata.voteAverage = 6.1;
    record.metadata.voteCount = 6400;
    record.metadata.genres = {"Thriller", "Mystery"};

    record.metadata.preferredArtwork.available = true;
    record.metadata.preferredArtwork.provider = "tvscraper";
    record.metadata.preferredArtwork.path =
        "/var/cache/vdr/plugins/tvscraper/movies/207932/poster.jpg";
    record.metadata.preferredArtwork.width = 1000;
    record.metadata.preferredArtwork.height = 1500;

    VdrRecordingNativePerson person;
    person.role = "actor";
    person.name = "Tom Hanks";
    person.characterName = "Robert Langdon";
    person.image.available = true;
    person.image.provider = "tvscraper";
    person.image.path =
        "/var/cache/vdr/plugins/tvscraper/actors/tom-hanks.jpg";
    record.metadata.people.push_back(person);

    VdrRecordingNativeArtwork image;
    image.available = true;
    image.provider = "tvscraper";
    image.path =
        "/var/cache/vdr/plugins/tvscraper/movies/207932/fanart.jpg";
    image.orientation = "landscape";
    record.metadata.images.push_back(image);

    const std::string json =
        VdrRecordingNativeMetadataPublicJsonSerializer().serialize(record);

    assert(json.find("\"available\":true") != std::string::npos);
    assert(json.find("\"provider\":\"tvscraper\"") != std::string::npos);
    assert(json.find("\"title\":\"Inferno\"") != std::string::npos);
    assert(json.find("\"name\":\"Tom Hanks\"") != std::string::npos);
    assert(json.find("\"characterName\":\"Robert Langdon\"") !=
        std::string::npos);
    assert(json.find(
        "/api/vdr/recordings/metadata/image?backend=default") !=
        std::string::npos);
    assert(json.find(
        "%2Fsrv%2Fvdr%2Fvideo%2FFilme%2FInferno") !=
        std::string::npos);
    assert(json.find(
        "/var/cache/vdr/plugins/tvscraper") ==
        std::string::npos);

    const VdrRecordingNativeMetadataRecord missing;
    assert(
        VdrRecordingNativeMetadataPublicJsonSerializer()
            .serialize(missing) ==
        "{\"available\":false,\"status\":\"not-found\"}");

    std::cout
        << "test_vdr_recording_native_metadata_public_json_serializer passed"
        << std::endl;

    return 0;
}
'''

    editor.create(
        "core/vdr/tests/"
        "test_vdr_recording_native_metadata_public_json_serializer.cpp",
        content,
    )


def patch_recording_metadata_tests(editor):
    editor.replace_once(
        "mk/recording-metadata-tests.mk",
        '''test-vdr-recording-artwork-service:
''',
        '''test-vdr-recording-native-metadata-public-json-serializer:
\t$(BUILD_CXX) $(CXXFLAGS) \\
\t\tcore/vdr/src/VdrRecordingNativeMetadataPublicJsonSerializer.cpp \\
\t\tcore/vdr/tests/test_vdr_recording_native_metadata_public_json_serializer.cpp \\
\t\t-o $(BUILD_DIR)/test_vdr_recording_native_metadata_public_json_serializer
\t$(BUILD_DIR)/test_vdr_recording_native_metadata_public_json_serializer

test-vdr-recording-artwork-service:
''',
    )

    editor.replace_once(
        "mk/recording-metadata-tests.mk",
        '''test-recording-metadata-foundation: \\
\ttest-restful-api-recording-metadata-mapper \\
\ttest-restful-api-recording-metadata-enricher \\
\ttest-vdr-recording-metadata-cache-codec \\
\ttest-vdr-recording-cache-metadata-persistence \\
\ttest-vdr-recording-metadata-json-serializer \\
\ttest-vdr-recording-artwork-service \\
\ttest-recording-artwork-http-server
''',
        '''test-recording-metadata-foundation: \\
\ttest-restful-api-recording-metadata-mapper \\
\ttest-restful-api-recording-metadata-enricher \\
\ttest-vdr-recording-metadata-cache-codec \\
\ttest-vdr-recording-cache-metadata-persistence \\
\ttest-vdr-recording-metadata-json-serializer \\
\ttest-vdr-recording-native-metadata-public-json-serializer \\
\ttest-vdr-recording-artwork-service \\
\ttest-recording-artwork-http-server
''',
    )

    editor.replace_once(
        "mk/recording-metadata-tests.mk",
        '''test-vdr-recording-folder-controller: CXXFLAGS += \\
\tcore/vdr/src/VdrRecordingMetadataCacheCodec.cpp \\
\tcore/vdr/src/VdrRecordingArtworkIdentity.cpp
''',
        '''test-vdr-recording-folder-controller: CXXFLAGS += \\
\tcore/vdr/src/VdrRecordingMetadataCacheCodec.cpp \\
\tcore/vdr/src/VdrRecordingArtworkIdentity.cpp \\
\tcore/vdr/src/VdrRecordingNativeMetadataPublicJsonSerializer.cpp \\
\tcore/vdr/src/EpgArtworkRepository.cpp \\
\tapi/rest/src/EpgArtworkController.cpp
''',
    )


patch.create_metadata_detail_module = create_metadata_detail_module
patch.create_serializer_test = create_serializer_test
patch.patch_recording_metadata_tests = patch_recording_metadata_tests

raise SystemExit(patch.main())
