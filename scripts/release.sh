#!/bin/sh
# GitHub release on a version tag (specs/001-releases): the Debian package
# built from a temporary worktree at the tag, SHA256SUMS, and as text the
# section of RELEASE-NOTES.adoc at the tag. The author pushes the tag; the
# script only checks it.
set -eu

tag=${1:-}
case $tag in
v[0-9]*.[0-9]*.[0-9]*) ;;
*) echo "usage: scripts/release.sh vX.Y.Z" >&2; exit 2 ;;
esac
version=${tag#v}

test "$(git cat-file -t "$tag" 2>/dev/null)" = tag ||
	{ echo "$tag is not an annotated tag" >&2; exit 1; }
git ls-remote --exit-code --tags origin "refs/tags/$tag" >/dev/null ||
	{ echo "tag $tag is not pushed to origin: git push origin $tag" >&2; exit 1; }
if gh release view "$tag" >/dev/null 2>&1; then
	echo "release $tag exists; to replace it: gh release delete $tag" >&2
	exit 1
fi

work=$(mktemp -d)
trap 'git worktree remove --force "$work/src" >/dev/null 2>&1 || true; rm -rf "$work"' EXIT INT TERM
git worktree add --quiet --detach "$work/src" "$tag"
cmake -S "$work/src" -B "$work/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr >/dev/null
cmake --build "$work/build" --parallel "$(nproc)"

# The version of the build is exactly the tag: the worktree is clean, and
# git describe gives the tag
got=$("$work/build/weather-test" --version | awk '{print $NF}')
test "$got" = "$tag" || { echo "the build reports version $got, not $tag" >&2; exit 1; }

(cd "$work/build" && cpack -G DEB -D CPACK_PACKAGE_VERSION="$version" >/dev/null)
deb="lxqt-weather_${version}_amd64.deb"
got=$(dpkg-deb -f "$work/build/$deb" Version)
test "$got" = "$version" || { echo "the package has version $got, not $version" >&2; exit 1; }

mkdir "$work/dist"
cp "$work/build/$deb" "$work/dist/"
(cd "$work/dist" && sha256sum "$deb" >SHA256SUMS)

url=$(gh repo view --json url -q .url)
git show "$tag:RELEASE-NOTES.adoc" >"$work/notes.adoc"
"$work/build/relnotes" "$work/notes.adoc" "$version" "$url/blob/$tag/" >"$work/notes.md"

gh release create "$tag" --verify-tag --title "lxqt-weather $version" --notes-file "$work/notes.md" "$work/dist"/*
