#!/bin/sh
# Helpers for CI/release: version checks, changelog notes, Debian version,
# and expected artifact names. Numeric version always comes from version.h.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
VERSION_H="$ROOT/libqnapi/src/version.h"
CHANGELOG="$ROOT/CHANGELOG.md"

numeric_version() {
  grep '^#define QNAPI_VERSION ' "$VERSION_H" | awk '{gsub(/"/, "", $3); print $3}'
}

header_displayable() {
  grep '^#define QNAPI_DISPLAYABLE_VERSION ' "$VERSION_H" | awk '{gsub(/"/, "", $3); print $3}'
}

displayable_version() {
  if [ -n "${QNAPI_DISPLAYABLE_VERSION:-}" ]; then
    printf '%s\n' "$QNAPI_DISPLAYABLE_VERSION"
  else
    header_displayable
  fi
}

# The part of TAG before the first '-' must equal QNAPI_VERSION.
# 0.3.0 and 0.3.0-rc1 match QNAPI_VERSION 0.3.0; 0.3.1-rc1 does not.
check_tag() {
  tag=$1
  ver=$(numeric_version)
  if [ -z "$tag" ]; then
    echo "empty tag" >&2
    return 1
  fi
  echo "$tag" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z][0-9A-Za-z.-]*)?$' || {
    echo "tag '$tag' is not major.minor.patch or major.minor.patch-<prerelease>" >&2
    return 1
  }
  prefix=${tag%%-*}
  if [ "$prefix" != "$ver" ]; then
    echo "tag '$tag' does not match QNAPI_VERSION '$ver' (expected '$ver' or '$ver-<prerelease>')" >&2
    return 1
  fi
}

is_prerelease() {
  tag=$1
  case "$tag" in
    *-*) return 0 ;;
    *) return 1 ;;
  esac
}

changelog_section() {
  ver=$1
  awk -v ver="$ver" '
    BEGIN { p = 0 }
    index($0, "## [" ver "]") == 1 { p = 1; print; next }
    p && $0 ~ /^## \[/ { exit }
    p { print }
  ' "$CHANGELOG"
  # fail if the section was missing
  awk -v ver="$ver" '
    index($0, "## [" ver "]") == 1 { found = 1 }
    END { if (!found) { exit 1 } }
  ' "$CHANGELOG" || {
    echo "no CHANGELOG.md section for [$ver]" >&2
    return 1
  }
}

release_notes() {
  tag=$1
  ver=${tag%%-*}
  if is_prerelease "$tag"; then
    cat <<EOF
This is a test build of QNapi ${ver} (tag ${tag}). It is published as a prerelease for manual testing; the final ${ver} release will follow from the same code.

EOF
  fi
  changelog_section "$ver"
}

# Debian upstream version: 0.3.0 stays 0.3.0; 0.3.0-rc1 becomes 0.3.0~rc1
# so the rc sorts before the final 0.3.0-1 package.
debian_upstream() {
  tag=$1
  case "$tag" in
    *-*) printf '%s~%s\n' "${tag%%-*}" "${tag#*-}" ;;
    *) printf '%s\n' "$tag" ;;
  esac
}

debian_version() {
  printf '%s-1\n' "$(debian_upstream "$1")"
}

# GitHub Release assets cannot contain '~' (it is rewritten to '.'). Keep the
# package version with '~' for dpkg ordering, but name the uploaded file with '.'.
debian_artifact_name() {
  printf 'qnapi_%s_amd64.deb\n' "$(debian_version "$1" | tr '~' '.')"
}

prepare_debian_changelog() {
  tag=$1
  debver=$(debian_version "$tag")
  current=$(dpkg-parsechangelog -l "$ROOT/debian/changelog" -S Version 2>/dev/null || true)
  if [ "$current" = "$debver" ]; then
    return 0
  fi
  maintainer='Mikołaj Stańczak <6730023+MikoWhy91@users.noreply.github.com>'
  date=$(date -uR)
  tmp=$(mktemp)
  cat > "$tmp" <<EOF
qnapi ($debver) unstable; urgency=medium

  * Package build of QNapi $tag.

 -- ${maintainer}  ${date}

EOF
  cat "$ROOT/debian/changelog" >> "$tmp"
  mv "$tmp" "$ROOT/debian/changelog"
}

expected_artifacts() {
  tag=$1
  cat <<EOF
QNapi-${tag}-setup.exe
QNapi-${tag}-portable.zip
QNapi-${tag}-x86_64.AppImage
QNapi-${tag}.dmg
qnapi-${tag}.tar.gz
$(debian_artifact_name "$tag")
SHA256SUMS
EOF
}

check_artifacts() {
  dir=$1
  tag=$2
  missing=0
  for f in $(expected_artifacts "$tag"); do
    if [ ! -f "$dir/$f" ]; then
      echo "missing artifact: $f" >&2
      missing=1
    fi
  done
  [ "$missing" -eq 0 ]
}

cmd=${1:-}
shift || true
case "$cmd" in
  numeric-version) numeric_version ;;
  displayable-version) displayable_version ;;
  check-tag) check_tag "${1:?tag}" ;;
  is-prerelease) is_prerelease "${1:?tag}" ;;
  changelog-section) changelog_section "${1:?version}" ;;
  release-notes) release_notes "${1:?tag}" ;;
  debian-version) debian_version "${1:?tag}" ;;
  debian-artifact-name) debian_artifact_name "${1:?tag}" ;;
  prepare-debian-changelog) prepare_debian_changelog "${1:?tag}" ;;
  expected-artifacts) expected_artifacts "${1:?tag}" ;;
  check-artifacts) check_artifacts "${1:?dir}" "${2:?tag}" ;;
  *)
    echo "usage: $0 numeric-version|displayable-version|check-tag TAG|is-prerelease TAG|changelog-section VER|release-notes TAG|debian-version TAG|debian-artifact-name TAG|prepare-debian-changelog TAG|expected-artifacts TAG|check-artifacts DIR TAG" >&2
    exit 2
    ;;
esac
