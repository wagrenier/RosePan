#!/usr/bin/env bash
# Print the next release as "key=value" lines for $GITHUB_OUTPUT.
#
# The last release is the highest vMAJOR.MINOR.PATCH tag anywhere in the repository; without
# one the count starts from 0.0.0. With --if-changed the answer is release=false when nothing
# that ships differs from that release, so a docs or tooling push does not use up a number.
set -euo pipefail

usage="Usage: next-version.sh patch|minor|major [--if-changed]"
bump=${1:?$usage}
only_if_changed=${2:-}

# What ends up in a build; everything else in the repository is docs and reversing tools.
shipped=(CMakeLists.txt cmake extern resources src)

previous=$(git tag --list 'v*' |
    { grep -E '^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$' || true; } |
    sort -V | tail -n1)
IFS=. read -r major minor patch <<<"${previous#v}"
major=${major:-0} minor=${minor:-0} patch=${patch:-0}

case $bump in
    major) major=$((major + 1)) minor=0 patch=0 ;;
    minor) minor=$((minor + 1)) patch=0 ;;
    patch) patch=$((patch + 1)) ;;
    *) echo "$usage" >&2; exit 2 ;;
esac

release=true
case $only_if_changed in
    '') ;;
    --if-changed)
        if [ -n "$previous" ] && git diff --quiet "$previous" HEAD -- "${shipped[@]}"; then
            release=false
        fi
        ;;
    *) echo "$usage" >&2; exit 2 ;;
esac

echo "release=$release"
echo "version=$major.$minor.$patch"
echo "tag=v$major.$minor.$patch"
echo "previous=$previous"
