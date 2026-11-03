#!/usr/bin/env bash
# RETIRED: historical yaVDR Media Tools kiss snapshot installer.
# These pinned historical builds predate the merged Main runtime and may
# replace newer installed SSE/Media-Tools files. They must not be reused.
# This script deliberately makes NO Git, build, service, permission or
# installation changes. It does not switch Unix users.
printf '%s\n' 'MT_INSTALL=STOP_LEGACY_INSTALLER_RETIRED' \
  'This historical installer is not a supported update path after PR #556.' \
  'Use an independently reviewed current-main staging/install procedure.' >&2
return 2 2>/dev/null || exit 2
