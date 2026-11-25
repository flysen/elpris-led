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

# Stable releases are built only from code that has been merged to main (and so passed CI). Release
# candidates may also be tagged on a pull request's branch, so a change can run on a lamp with the test
# channel before it is merged.
git fetch --quiet origin main
if [[ "$TAG" != *-* ]] && ! git merge-base --is-ancestor HEAD origin/main; then
  echo "::error::Tag $TAG points to a commit that is not on main. Only release candidates (-rcN) may be tagged on a branch."
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
  # The tagged commit must describe this release in CHANGELOG.md, so a tag set before the pull request
  # with the changes was merged (as happened with v5.1.2 and v5.1.3) stops here
  if ! grep -qE "^## (.* )?${TAG//./\\.}( .*)?$" CHANGELOG.md; then
    echo "::error::CHANGELOG.md on the tagged commit has no heading for $TAG. Merge the pull request that renames Unreleased to $TAG first, then tag."
    exit 1
  fi
  echo "Stable release $TAG (previous: ${PREV:-none})"
else
  echo "Release candidate $TAG: published as a pre-release, lamps will not install it automatically"
fi
