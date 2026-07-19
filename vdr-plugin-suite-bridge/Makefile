PLUGIN = suitebridge

VERSION = $(shell sed -n 's/^static const char \*VERSION[[:space:]]*=[[:space:]]*"\([^"]*\)";/\1/p' $(PLUGIN).cpp)

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

OBJS = $(PLUGIN).o suitebridge_lifecycle.o suitebridge_capabilities.o suitebridge_capability_discovery.o suitebridge_counter_continuity.o suitebridge_status_snapshot.o suitebridge_local_contract.o suitebridge_svdrp_contract.o suitebridge_status_events.o suitebridge_status_monitor.o suitebridge_artwork_reference.o suitebridge_tvscraper_adapter.o suitebridge_epg_artwork_contract.o
DEPFILE = .dependencies
LIFECYCLE_TEST = /tmp/test_suitebridge_lifecycle
CAPABILITIES_TEST = /tmp/test_suitebridge_capabilities
CAPABILITY_DISCOVERY_TEST = /tmp/test_suitebridge_capability_discovery
COUNTER_CONTINUITY_TEST = /tmp/test_suitebridge_counter_continuity
STATUS_EVENTS_TEST = /tmp/test_suitebridge_status_events
STATUS_SNAPSHOT_TEST = /tmp/test_suitebridge_status_snapshot
LOCAL_CONTRACT_TEST = /tmp/test_suitebridge_local_contract
SVDRP_CONTRACT_TEST = /tmp/test_suitebridge_svdrp_contract
ARTWORK_REFERENCE_TEST = /tmp/test_suitebridge_artwork_reference
EPG_ARTWORK_CONTRACT_TEST = /tmp/test_suitebridge_epg_artwork_contract

.PHONY: all install install-lib check check-contract check-capabilities-contract check-capability-discovery-contract check-counter-continuity-contract check-status-events-contract check-status-snapshot-contract check-local-contract-contract check-svdrp-contract check-version test-lifecycle test-capabilities test-capability-discovery test-counter-continuity test-status-events test-status-snapshot test-local-contract test-svdrp-contract test-artwork-reference test-epg-artwork-contract check-build clean

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

check-version:
	test "$(VERSION)" = "0.10.0"

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

test-artwork-reference:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_artwork_reference.cpp \
		tests/test_suitebridge_artwork_reference.cpp \
		-o $(ARTWORK_REFERENCE_TEST)
	$(ARTWORK_REFERENCE_TEST)
	rm -f $(ARTWORK_REFERENCE_TEST)

test-epg-artwork-contract:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_artwork_reference.cpp \
		suitebridge_epg_artwork_contract.cpp \
		tests/test_suitebridge_epg_artwork_contract.cpp \
		-o $(EPG_ARTWORK_CONTRACT_TEST)
	$(EPG_ARTWORK_CONTRACT_TEST)
	rm -f $(EPG_ARTWORK_CONTRACT_TEST)

check-build: $(SOFILE)
	test -s $(SOFILE)
	readelf -h $(SOFILE) >/dev/null

check: check-contract check-capabilities-contract check-capability-discovery-contract check-counter-continuity-contract check-status-events-contract check-status-snapshot-contract check-local-contract-contract check-svdrp-contract check-version test-lifecycle test-capabilities test-capability-discovery test-counter-continuity test-status-events test-status-snapshot test-local-contract test-svdrp-contract test-artwork-reference test-epg-artwork-contract check-build

clean:
	rm -f $(OBJS) $(DEPFILE) $(SOFILE) $(LIFECYCLE_TEST) $(CAPABILITIES_TEST) $(CAPABILITY_DISCOVERY_TEST) $(COUNTER_CONTINUITY_TEST) $(STATUS_EVENTS_TEST) $(STATUS_SNAPSHOT_TEST) $(LOCAL_CONTRACT_TEST) $(SVDRP_CONTRACT_TEST) $(ARTWORK_REFERENCE_TEST) $(EPG_ARTWORK_CONTRACT_TEST) *.tgz core* *~ ._*
