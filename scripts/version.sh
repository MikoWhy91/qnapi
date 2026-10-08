#!/bin/sh
# Prints the displayable version. QNAPI_DISPLAYABLE_VERSION in the environment
# wins (release builds pass the git tag); otherwise version.h is used.
if [ -n "$QNAPI_DISPLAYABLE_VERSION" ]; then
  printf '%s\n' "$QNAPI_DISPLAYABLE_VERSION"
  exit 0
fi
VERSION_FILE=libqnapi/src/version.h
if [ ! -f "$VERSION_FILE" ]; then
  VERSION_FILE="$(dirname "$0")/../libqnapi/src/version.h"
fi
(grep '^#define QNAPI_DISPLAYABLE_VERSION ' | awk '{gsub(/"/, "", $3); print $3}') < "$VERSION_FILE"
