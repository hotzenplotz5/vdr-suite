#!/bin/sh
# MLD.1A: optional read-only target inventory; run only on a consenting MLD test host.
# Invocation: sh tools/mld66-readonly-inventory.sh [--caps]
# Never reads configuration contents, credential stores, databases or recordings.
set -eu

case "${1-}" in
  "") caps=0 ;;
  --caps) caps=1 ;;
  *) printf 'Usage: sh tools/mld66-readonly-inventory.sh [--caps]\n' >&2; exit 2 ;;
esac
if [ "$#" -gt 1 ]; then
  printf 'Too many arguments\n' >&2
  exit 2
fi

echo 'MLD66_INVENTORY=READ_ONLY'
printf 'HOST_ARCH='
uname -m
printf 'KERNEL='
uname -r

if [ -r /etc/os-release ]; then
  # Distribution name/version only; no arbitrary environment/config values.
  sed -n '/^ID=/p; /^VERSION_ID=/p' /etc/os-release
else
  echo 'OS_RELEASE=UNAVAILABLE'
fi

for cmd in vdr pkg-config svdrpsend svdrpsend.sh systemctl apt opkg rpm ffmpeg ffprobe; do
  if command -v "$cmd" >/dev/null 2>&1; then
    printf 'COMMAND_%s=PRESENT\n' "$cmd"
  else
    printf 'COMMAND_%s=ABSENT\n' "$cmd"
  fi
done

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists vdr; then
  for key in modversion apiversion libdir; do
    if [ "$key" = modversion ]; then
      value=$(pkg-config --modversion vdr 2>/dev/null || printf UNKNOWN)
    else
      value=$(pkg-config --variable="$key" vdr 2>/dev/null || printf UNKNOWN)
    fi
    printf 'VDR_PKGCONFIG_%s=%s\n' "$key" "$value"
  done
else
  echo 'VDR_PKGCONFIG=UNAVAILABLE_ON_HOST'
  echo 'VDR_ABI=REQUIRES_BUILD_SYSROOT_EVIDENCE'
fi

# Avoid ps args/environ: both can include secrets. Only the process command name
# and uid are sampled, and absence of procps output is not evidence of no VDR.
if command -v ps >/dev/null 2>&1; then
  p=$(ps -eo comm=,user= 2>/dev/null | awk '$1 == "vdr" {print $2}' | sort -u || true)
  if [ -n "$p" ]; then
    printf 'VDR_PROCESS_USERS=%s\n' "$p"
  else
    echo 'VDR_PROCESS_USERS=UNKNOWN'
  fi
fi

# This request is explicitly opt-in and read-only. A bounded timeout is
# mandatory; no mutation, timer creation or RESTfulAPI probing.
if [ "$caps" -eq 1 ]; then
  if command -v timeout >/dev/null 2>&1 && command -v svdrpsend.sh >/dev/null 2>&1; then
    echo 'SUITEBRIDGE_CAPS_BEGIN'
    timeout 8 svdrpsend.sh plug suitebridge CAPS 1 || echo 'SUITEBRIDGE_CAPS=FAILED_OR_UNAVAILABLE'
    echo 'SUITEBRIDGE_CAPS_END'
  else
    echo 'SUITEBRIDGE_CAPS=SKIPPED_REQUIRES_TIMEOUT_AND_SVDRPSEND_SH'
  fi
fi
echo 'MLD66_INVENTORY=END'
