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

OBJS = $(PLUGIN).o suitebridge_lifecycle.o
DEPFILE = .dependencies
LIFECYCLE_TEST = /tmp/test_suitebridge_lifecycle

.PHONY: all install install-lib check check-contract check-version test-lifecycle check-build clean

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

check-version:
	test "$(VERSION)" = "0.2.0"

test-lifecycle:
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -I. \
		suitebridge_lifecycle.cpp \
		tests/test_suitebridge_lifecycle.cpp \
		-o $(LIFECYCLE_TEST)
	$(LIFECYCLE_TEST)
	rm -f $(LIFECYCLE_TEST)

check-build: $(SOFILE)
	test -s $(SOFILE)
	readelf -h $(SOFILE) >/dev/null

check: check-contract check-version test-lifecycle check-build

clean:
	rm -f $(OBJS) $(DEPFILE) $(SOFILE) $(LIFECYCLE_TEST) *.tgz core* *~ ._*
