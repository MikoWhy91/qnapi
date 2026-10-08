#!/bin/sh
# Self-check for scripts/release-meta.sh. Run from the repo root or via
# `sh scripts/release-meta.test.sh`.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"
META="$ROOT/scripts/release-meta.sh"
fail=0

assert_eq() {
  got=$1
  want=$2
  msg=$3
  if [ "$got" != "$want" ]; then
    echo "FAIL: $msg (got '$got', want '$want')" >&2
    fail=1
  fi
}

assert_ok() {
  msg=$1
  shift
  if ! "$@" >/dev/null; then
    echo "FAIL: $msg (expected success)" >&2
    fail=1
  fi
}

assert_fail() {
  msg=$1
  shift
  if "$@" >/dev/null 2>&1; then
    echo "FAIL: $msg (expected failure)" >&2
    fail=1
  fi
}

VER=$(sh "$META" numeric-version)
assert_eq "$VER" "0.3.0" "QNAPI_VERSION"

assert_ok "tag 0.3.0" sh "$META" check-tag 0.3.0
assert_ok "tag 0.3.0-rc1" sh "$META" check-tag 0.3.0-rc1
assert_fail "tag 0.3.1-rc1" sh "$META" check-tag 0.3.1-rc1
assert_fail "tag 0.2.3" sh "$META" check-tag 0.2.3
assert_fail "tag v0.3.0" sh "$META" check-tag v0.3.0

if sh "$META" is-prerelease 0.3.0-rc1; then
  :
else
  echo "FAIL: 0.3.0-rc1 should be a prerelease" >&2
  fail=1
fi
if sh "$META" is-prerelease 0.3.0; then
  echo "FAIL: 0.3.0 should not be a prerelease" >&2
  fail=1
fi

assert_eq "$(sh "$META" debian-version 0.3.0)" "0.3.0-1" "debian 0.3.0"
assert_eq "$(sh "$META" debian-version 0.3.0-rc1)" "0.3.0~rc1-1" "debian 0.3.0-rc1"

notes_rc=$(sh "$META" release-notes 0.3.0-rc1)
echo "$notes_rc" | grep -q "test build of QNapi 0.3.0" || {
  echo "FAIL: rc notes missing test-build sentence" >&2
  fail=1
}
echo "$notes_rc" | grep -q "OpenSubtitles.com REST API" || {
  echo "FAIL: rc notes missing 0.3.0 changelog body" >&2
  fail=1
}
echo "$notes_rc" | grep -Eq 'This is a test build' || {
  echo "FAIL: rc notes missing English test-build note" >&2
  fail=1
}

notes_final=$(sh "$META" release-notes 0.3.0)
echo "$notes_final" | grep -q "test build" && {
  echo "FAIL: final 0.3.0 notes must not include the test-build note" >&2
  fail=1
}
echo "$notes_final" | grep -q "## \[0.3.0\]" || {
  echo "FAIL: final notes missing ## [0.3.0] heading" >&2
  fail=1
}
echo "$notes_final" | grep -q "^## \[0.2.3\]" && {
  echo "FAIL: 0.3.0 notes should not include the 0.2.3 section" >&2
  fail=1
}

# Artifact names for both styles
sh "$META" expected-artifacts 0.3.0-rc1 | grep -qx 'QNapi-0.3.0-rc1-setup.exe' || {
  echo "FAIL: rc setup.exe name" >&2
  fail=1
}
sh "$META" expected-artifacts 0.3.0-rc1 | grep -qx 'qnapi_0.3.0~rc1-1_amd64.deb' || {
  echo "FAIL: rc deb name" >&2
  fail=1
}
sh "$META" expected-artifacts 0.3.0 | grep -qx 'QNapi-0.3.0.dmg' || {
  echo "FAIL: final dmg name" >&2
  fail=1
}
sh "$META" expected-artifacts 0.3.0 | grep -qx 'qnapi-0.3.0.tar.gz' || {
  echo "FAIL: final tarball name" >&2
  fail=1
}

# English-only changelog / notes (no remaining Polish changelog phrasing)
if echo "$notes_rc" | grep -Eq 'naprawiony|przebudowany|mozliwosc'; then
  echo "FAIL: release notes must be English" >&2
  fail=1
fi

if [ "$fail" -ne 0 ]; then
  echo "release-meta tests FAILED" >&2
  exit 1
fi
echo "release-meta tests OK (numeric $VER; rc and final names/notes checked)"
