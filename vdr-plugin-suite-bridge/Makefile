PLUGIN = suitebridge

VERSION = $(shell sed -n 's/^inline constexpr const char \*Version[[:space:]]*=[[:space:]]*"\([^"]*\)";/\1/p' suitebridge_plugin_identity.h)

PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
PLGCFG = $(call PKGCFG,plgcfg)
TMPDIR ?= /tmp

export CFLAGS = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags) -std=c++17

APIVERSION = $(call PKGCFG,apiversion)

-include $(PLGCFG)

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)
SOFILE = libvdr-$(PLUGIN).so

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

OBJS = $(PLUGIN).o suitebridge_svdrp.o suitebridge_native_probe.o suitebridge_native_timer_create.o suitebridge_native_timer_delete.o suitebridge_native_timer_delete_vdr.o suitebridge_epg_command_handler.o suitebridge_epg_type_snapshot_command.o suitebridge_recording_metadata_command.o suitebridge_lifecycle.o suitebridge_capabilities.o suitebridge_capability_discovery.o suitebridge_counter_continuity.o suitebridge_status_snapshot.o suitebridge_local_contract.o suitebridge_svdrp_contract.o suitebridge_status_events.o suitebridge_status_monitor.o suitebridge_artwork_reference.o suitebridge_image_dimensions.o suitebridge_tvscraper_adapter.o suitebridge_tvscraper_type_adapter.o suitebridge_epg_artwork_contract.o suitebridge_epg_metadata_contract.o suitebridge_epg_type_snapshot_contract.o suitebridge_recording_identity.o suitebridge_recording_metadata.o suitebridge_recording_metadata_contract.o suitebridge_tvscraper_recording_adapter.o
DEPFILE = .dependencies
LIFECYCLE_TEST = /tmp/test_suitebridge_lifecycle
CAPABILITIES_TEST = /tmp/test_suitebridge_capabilities
CAPABILITY_DISCOVERY_TEST = /tmp/test_suitebridge_capability_discovery
COUNTER_CONTINUITY_TEST = /tmp/test_suitebridge_counter_continuity
STATUS_EVENTS_TEST = /tmp/test_suitebridge_status_events
STATUS_SNAPSHOT_TEST = /tmp/test_suitebridge_status_snapshot
LOCAL_CONTRACT_TEST = /tmp/test_suitebridge_local_contract
SVDRP_CONTRACT_TEST = /tmp/test_suitebridge_svdrp_contract
NATIVE_PROBE_TEST = /tmp/test_suitebridge_native_probe
NATIVE_TIMER_CREATE_TEST = /tmp/test_suitebridge_native_timer_create
NATIVE_TIMER_DELETE_TEST = /tmp/test_suitebridge_native_timer_delete
ARTWORK_REFERENCE_TEST = /tmp/test_suitebridge_artwork_reference
IMAGE_DIMENSIONS_TEST = /tmp/test_suitebridge_image_dimensions
EPG_ARTWORK_CONTRACT_TEST = /tmp/test_suitebridge_epg_artwork_contract
EPG_METADATA_CONTRACT_TEST = /tmp/test_suitebridge_epg_metadata_contract
EPG_TYPE_SNAPSHOT_CONTRACT_TEST = /tmp/test_suitebridge_epg_type_snapshot_contract
RECORDING_IDENTITY_TEST = /tmp/test_suitebridge_recording_identity
RECORDING_METADATA_CONTRACT_TEST = /tmp/test_suitebridge_recording_metadata_contract

.PHONY: all install install-lib check check-contract check-capabilities-contract check-capability-discovery-contract check-counter-continuity-contract check-status-events-contract check-status-snapshot-contract check-local-contract-contract check-svdrp-contract check-recording-metadata-contract check-version test-lifecycle test-capabilities test-capability-discovery test-counter-continuity test-status-events test-status-snapshot test-local-contract test-svdrp-contract test-native-probe test-native-timer-create test-native-timer-delete test-artwork-reference test-image-dimensions test-epg-artwork-contract test-epg-metadata-contract test-epg-type-snapshot-contract test-recording-identity test-recording-metadata-contract check-build clean

all: $(SOFILE)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

$(DEPFILE): Makefile $(OBJS:%.o=%.cpp)
	@$(CXX) -MM -MG $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

$(SOFILE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) -o $@ $(LIBS)

install-lib: $(SOFILE)
	install -D -m 0755 $(SOFILE) $(DESTDIR)$(LIBDIR)/$(SOFILE).$(APIVERSION)

install: install-lib

check-contract:
	python3 tests/check_foundation_contract.py

check-capabilities-contract:
	python3 tests/check_capabilities_contract.py

check-capability-discovery-contract:
	python3 tests/check_capability_discovery_contract.py

check-counter-continuity-contract:
	python3 tests/check_counter_continuity_contract.py

check-status-events-contract:
	python3 tests/check_status_events_contract.py

check-status-snapshot-contract:
	python3 tests/check_status_snapshot_contract.py

check-local-contract-contract:
	python3 tests/check_local_contract_contract.py

check-svdrp-contract:
	python3 tests/check_svdrp_contract.py

check-recording-metadata-contract:
	python3 tests/check_recording_metadata_contract.py

check-version:
	test "$(VERSION)" = "0.13.3"

test-lifecycle:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_lifecycle.cpp \
		tests/test_suitebridge_lifecycle.cpp \
		-o $(LIFECYCLE_TEST)
	$(LIFECYCLE_TEST)
	rm -f $(LIFECYCLE_TEST)

test-capabilities:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_capabilities.cpp \
		tests/test_suitebridge_capabilities.cpp \
		-o $(CAPABILITIES_TEST)
	$(CAPABILITIES_TEST)
	rm -f $(CAPABILITIES_TEST)

test-capability-discovery:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_capabilities.cpp \
		suitebridge_capability_discovery.cpp \
		tests/test_suitebridge_capability_discovery.cpp \
		-o $(CAPABILITY_DISCOVERY_TEST)
	$(CAPABILITY_DISCOVERY_TEST)
	rm -f $(CAPABILITY_DISCOVERY_TEST)

test-counter-continuity:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_counter_continuity.cpp \
		tests/test_suitebridge_counter_continuity.cpp \
		-o $(COUNTER_CONTINUITY_TEST)
	$(COUNTER_CONTINUITY_TEST)
	rm -f $(COUNTER_CONTINUITY_TEST)

test-status-events:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_counter_continuity.cpp \
		suitebridge_status_snapshot.cpp \
		suitebridge_status_events.cpp \
		tests/test_suitebridge_status_events.cpp \
		-o $(STATUS_EVENTS_TEST)
	$(STATUS_EVENTS_TEST)
	rm -f $(STATUS_EVENTS_TEST)

test-status-snapshot:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_counter_continuity.cpp \
		suitebridge_status_snapshot.cpp \
		suitebridge_status_events.cpp \
		tests/test_suitebridge_status_snapshot.cpp \
		-o $(STATUS_SNAPSHOT_TEST)
	$(STATUS_SNAPSHOT_TEST)
	rm -f $(STATUS_SNAPSHOT_TEST)

test-local-contract:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_status_snapshot.cpp \
		suitebridge_local_contract.cpp \
		tests/test_suitebridge_local_contract.cpp \
		-o $(LOCAL_CONTRACT_TEST)
	$(LOCAL_CONTRACT_TEST)
	rm -f $(LOCAL_CONTRACT_TEST)

test-svdrp-contract:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_status_snapshot.cpp \
		suitebridge_local_contract.cpp \
		suitebridge_svdrp_contract.cpp \
		tests/test_suitebridge_svdrp_contract.cpp \
		-o $(SVDRP_CONTRACT_TEST)
	$(SVDRP_CONTRACT_TEST)
	rm -f $(SVDRP_CONTRACT_TEST)

test-native-probe:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_native_probe.cpp \
		tests/test_suitebridge_native_probe.cpp \
		-o $(NATIVE_PROBE_TEST)
	$(NATIVE_PROBE_TEST)
	rm -f $(NATIVE_PROBE_TEST)

test-native-timer-create:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_native_timer_create.cpp \
		tests/test_suitebridge_native_timer_create.cpp \
		-o $(NATIVE_TIMER_CREATE_TEST)
	$(NATIVE_TIMER_CREATE_TEST)
	rm -f $(NATIVE_TIMER_CREATE_TEST)

test-native-timer-delete:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_native_timer_delete.cpp \
		tests/test_suitebridge_native_timer_delete.cpp \
		-o $(NATIVE_TIMER_DELETE_TEST)
	$(NATIVE_TIMER_DELETE_TEST)
	rm -f $(NATIVE_TIMER_DELETE_TEST)

test-artwork-reference:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_artwork_reference.cpp \
		tests/test_suitebridge_artwork_reference.cpp \
		-o $(ARTWORK_REFERENCE_TEST)
	$(ARTWORK_REFERENCE_TEST)
	rm -f $(ARTWORK_REFERENCE_TEST)

test-image-dimensions:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_image_dimensions.cpp \
		tests/test_suitebridge_image_dimensions.cpp \
		-o $(IMAGE_DIMENSIONS_TEST)
	$(IMAGE_DIMENSIONS_TEST)
	rm -f $(IMAGE_DIMENSIONS_TEST)

test-epg-artwork-contract:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_artwork_reference.cpp \
		suitebridge_epg_artwork_contract.cpp \
		tests/test_suitebridge_epg_artwork_contract.cpp \
		-o $(EPG_ARTWORK_CONTRACT_TEST)
	$(EPG_ARTWORK_CONTRACT_TEST)
	rm -f $(EPG_ARTWORK_CONTRACT_TEST)

test-epg-metadata-contract:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_artwork_reference.cpp \
		suitebridge_epg_metadata_contract.cpp \
		tests/test_suitebridge_epg_metadata_contract.cpp \
		-o $(EPG_METADATA_CONTRACT_TEST)
	$(EPG_METADATA_CONTRACT_TEST)
	rm -f $(EPG_METADATA_CONTRACT_TEST)

test-epg-type-snapshot-contract:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_epg_type_snapshot_contract.cpp \
		tests/test_suitebridge_epg_type_snapshot_contract.cpp \
		-o $(EPG_TYPE_SNAPSHOT_CONTRACT_TEST)
	$(EPG_TYPE_SNAPSHOT_CONTRACT_TEST)
	rm -f $(EPG_TYPE_SNAPSHOT_CONTRACT_TEST)

test-recording-identity:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_recording_identity.cpp \
		tests/test_suitebridge_recording_identity.cpp \
		-o $(RECORDING_IDENTITY_TEST)
	$(RECORDING_IDENTITY_TEST)
	rm -f $(RECORDING_IDENTITY_TEST)

test-recording-metadata-contract:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_artwork_reference.cpp \
		suitebridge_recording_identity.cpp \
		suitebridge_recording_metadata.cpp \
		suitebridge_recording_metadata_contract.cpp \
		tests/test_suitebridge_recording_metadata_contract.cpp \
		-o $(RECORDING_METADATA_CONTRACT_TEST)
	$(RECORDING_METADATA_CONTRACT_TEST)
	rm -f $(RECORDING_METADATA_CONTRACT_TEST)

check-build: $(SOFILE)
	test -s $(SOFILE)
	readelf -h $(SOFILE) >/dev/null

check: check-contract check-capabilities-contract check-capability-discovery-contract check-counter-continuity-contract check-status-events-contract check-status-snapshot-contract check-local-contract-contract check-svdrp-contract check-recording-metadata-contract check-version test-lifecycle test-capabilities test-capability-discovery test-counter-continuity test-status-events test-status-snapshot test-local-contract test-svdrp-contract test-native-probe test-native-timer-create test-native-timer-delete test-artwork-reference test-image-dimensions test-epg-artwork-contract test-epg-metadata-contract test-epg-type-snapshot-contract test-recording-identity test-recording-metadata-contract check-build

clean:
	rm -f $(OBJS) $(DEPFILE) $(SOFILE) $(LIFECYCLE_TEST) $(CAPABILITIES_TEST) $(CAPABILITY_DISCOVERY_TEST) $(COUNTER_CONTINUITY_TEST) $(STATUS_EVENTS_TEST) $(STATUS_SNAPSHOT_TEST) $(LOCAL_CONTRACT_TEST) $(SVDRP_CONTRACT_TEST) $(NATIVE_PROBE_TEST) $(NATIVE_TIMER_CREATE_TEST) $(NATIVE_TIMER_DELETE_TEST) $(ARTWORK_REFERENCE_TEST) $(IMAGE_DIMENSIONS_TEST) $(EPG_ARTWORK_CONTRACT_TEST) $(EPG_METADATA_CONTRACT_TEST) $(EPG_TYPE_SNAPSHOT_CONTRACT_TEST) $(RECORDING_IDENTITY_TEST) $(RECORDING_METADATA_CONTRACT_TEST) *.tgz core* *~ ._*
