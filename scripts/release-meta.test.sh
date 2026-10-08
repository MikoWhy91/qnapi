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
echo "$VER" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$' || {
  echo "FAIL: QNAPI_VERSION '$VER' is not major.minor.patch" >&2
  fail=1
}

assert_ok "tag $VER" sh "$META" check-tag "$VER"
assert_ok "tag ${VER}-rc1" sh "$META" check-tag "${VER}-rc1"
assert_fail "mismatched prerelease 9.9.9-rc1" sh "$META" check-tag 9.9.9-rc1
assert_fail "unrelated version 0.0.1" sh "$META" check-tag 0.0.1
assert_fail "v prefix" sh "$META" check-tag "v${VER}"

if sh "$META" is-prerelease "${VER}-rc1"; then
  :
else
  echo "FAIL: ${VER}-rc1 should be a prerelease" >&2
  fail=1
fi
if sh "$META" is-prerelease "$VER"; then
  echo "FAIL: $VER should not be a prerelease" >&2
  fail=1
fi

assert_eq "$(sh "$META" debian-version "$VER")" "${VER}-1" "debian $VER"
assert_eq "$(sh "$META" debian-version "${VER}-rc1")" "${VER}~rc1-1" "debian ${VER}-rc1"
assert_eq "$(sh "$META" debian-artifact-name "$VER")" "qnapi_${VER}-1_amd64.deb" "deb filename $VER"
assert_eq "$(sh "$META" debian-artifact-name "${VER}-rc1")" "qnapi_${VER}.rc1-1_amd64.deb" "deb filename ${VER}-rc1"

notes_rc=$(sh "$META" release-notes "${VER}-rc1")
echo "$notes_rc" | grep -q "test build of QNapi ${VER}" || {
  echo "FAIL: rc notes missing test-build sentence" >&2
  fail=1
}
echo "$notes_rc" | grep -q "OpenSubtitles.com REST API" || {
  echo "FAIL: rc notes missing ${VER} changelog body" >&2
  fail=1
}
echo "$notes_rc" | grep -Eq 'This is a test build' || {
  echo "FAIL: rc notes missing English test-build note" >&2
  fail=1
}

notes_final=$(sh "$META" release-notes "$VER")
echo "$notes_final" | grep -q "test build" && {
  echo "FAIL: final $VER notes must not include the test-build note" >&2
  fail=1
}
echo "$notes_final" | grep -q "## \[${VER}\]" || {
  echo "FAIL: final notes missing ## [$VER] heading" >&2
  fail=1
}
echo "$notes_final" | grep -q "^## \[0.2.3\]" && {
  echo "FAIL: $VER notes should not include the 0.2.3 section" >&2
  fail=1
}

sh "$META" expected-artifacts "${VER}-rc1" | grep -qx "QNapi-${VER}-rc1-setup.exe" || {
  echo "FAIL: rc setup.exe name" >&2
  fail=1
}
sh "$META" expected-artifacts "${VER}-rc1" | grep -qx "qnapi_${VER}.rc1-1_amd64.deb" || {
  echo "FAIL: rc deb release filename should use '.' not '~'" >&2
  fail=1
}
sh "$META" expected-artifacts "${VER}-rc1" | grep -q '~' && {
  echo "FAIL: expected-artifacts for rc must not contain '~'" >&2
  fail=1
}
sh "$META" expected-artifacts "$VER" | grep -qx "QNapi-${VER}.dmg" || {
  echo "FAIL: final dmg name" >&2
  fail=1
}
sh "$META" expected-artifacts "$VER" | grep -qx "qnapi-${VER}.tar.gz" || {
  echo "FAIL: final tarball name" >&2
  fail=1
}

if echo "$notes_rc" | grep -Eq 'naprawiony|przebudowany|mozliwosc'; then
  echo "FAIL: release notes must be English" >&2
  fail=1
fi

if [ "$fail" -ne 0 ]; then
  echo "release-meta tests FAILED" >&2
  exit 1
fi
echo "release-meta tests OK (numeric $VER; rc and final names/notes checked)"
