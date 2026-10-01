#!/usr/bin/env bash
# Tests scripts/check-specs.sh (specs/003-spec-checks): a valid specs/ passes,
# and every kind of violation fails with its own message.
set -eu

results='Results'         # as in check-specs.sh
bad_issue='not yet opened' # an :issue: that check-specs.sh rejects

check=$(cd "$(dirname "$0")" && pwd)/check-specs.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0

# spec <directory> <status> <issue> [results]: writes the spec.adoc
spec() {
	mkdir -p "$tmp/specs/$1"
	printf '= %s\n:revdate: 2026-10-01\n:status: %s\n:issue: %s\n\n== What and why\n' \
		"$1" "$2" "$3" >"$tmp/specs/$1/spec.adoc"
	if [[ ${4-} ]]; then
		printf '\n== %s\n' "$results" >>"$tmp/specs/$1/spec.adoc"
	fi
}

# valid: a specs/ with every status
valid() {
	rm -rf "$tmp/specs"
	mkdir -p "$tmp/specs"
	touch "$tmp/specs/README.adoc" "$tmp/specs/constitution.adoc"
	spec 001-done implemented 1 results
	spec 002-planned draft 2
	spec 003-abandoned dropped 3
}

# expect <case> <message>: the check passes when the message is empty, or
# fails and prints the message
expect() {
	local out
	if out=$("$check" "$tmp" 2>&1); then
		if [[ $2 ]]; then
			echo "FAIL $1: passed, expected: $2"
			fail=1
		fi
	elif [[ ! $2 || $out != *"$2"* ]]; then
		echo "FAIL $1: expected \"$2\", got: $out"
		fail=1
	fi
}

valid
expect valid ''

valid
touch "$tmp/specs/notes.adoc"
expect stray-file 'specs/notes.adoc: neither'

valid
mkdir "$tmp/specs/4-short"
expect bad-name 'specs/4-short: neither'

valid
spec 001-again draft 4
expect same-number 'specs/001-done: number 001 is taken'

valid
mkdir "$tmp/specs/004-empty"
expect no-spec 'specs/004-empty: no spec.adoc'

valid
spec 001-done 'implemented 2026-10-01; K1 met' 1 results
expect free-status 'specs/001-done/spec.adoc: :status: "implemented 2026-10-01; K1 met"'

valid
spec 001-done implemented 1
expect no-results "specs/001-done/spec.adoc: implemented, but no section \"$results\""

valid
spec 002-planned draft "$bad_issue"
expect bad-issue "specs/002-planned/spec.adoc: :issue: \"$bad_issue\""

valid
sed -i 's/^:status: draft$/:revdate: 2026-10-01/' "$tmp/specs/002-planned/spec.adoc"
echo ':status: draft' >>"$tmp/specs/002-planned/spec.adoc"
expect status-in-body 'specs/002-planned/spec.adoc: :status: ""'

exit $fail
