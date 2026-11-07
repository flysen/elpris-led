#!/usr/bin/env bash
# Checks a release tag before anything is built or published.
# Usage: scripts/check-release-tag.sh <tag>
set -euo pipefail

TAG="${1:?usage: $0 <tag>}"

# Format: v2.1, v2.1.3, or a release candidate like v2.1-rc1
if [[ ! "$TAG" =~ ^v[0-9]+\.[0-9]+(\.[0-9]+)?(-rc[0-9]+)?$ ]]; then
  echo "::error::Tag $TAG is not a valid version. Use v2.1, v2.1.3 or v2.1-rc1."
  exit 1
fi

# Releases are built only from code that has been merged to main (and so passed CI)
git fetch --quiet origin main
if ! git merge-base --is-ancestor HEAD origin/main; then
  echo "::error::Tag $TAG points to a commit that is not on main."
  exit 1
fi

# A stable release must be newer than every earlier stable release, or lamps will ignore it
if [[ "$TAG" != *-* ]]; then
  PREV=$(git tag --list 'v*' | grep -v -- '-' | grep -vxF "$TAG" | sort -V | tail -n 1 || true)
  if [ -n "$PREV" ]; then
    HIGHEST=$(printf '%s\n%s\n' "$PREV" "$TAG" | sort -V | tail -n 1)
    if [ "$HIGHEST" != "$TAG" ]; then
      echo "::error::Tag $TAG is not newer than the previous release $PREV."
      exit 1
    fi
  fi
  echo "Stable release $TAG (previous: ${PREV:-none})"
else
  echo "Release candidate $TAG: published as a pre-release, lamps will not install it automatically"
fi
