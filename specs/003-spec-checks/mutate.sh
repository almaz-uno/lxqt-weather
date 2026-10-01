#!/usr/bin/env bash
# Criterion K3 of specification 003: every mutant of scripts/check-specs.sh —
# one rule removed or weakened — makes scripts/check-specs-test.sh fail.
# Prints a line a mutant; the exit status is 1 if any survived.
set -eu

cd "$(dirname "$0")/../.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
survived=0

while IFS= read -r edit; do
	sed "$edit" scripts/check-specs.sh >"$tmp/check-specs.sh"
	if cmp -s scripts/check-specs.sh "$tmp/check-specs.sh"; then
		echo "not applied: $edit"
		survived=1
		continue
	fi
	cp scripts/check-specs-test.sh "$tmp/"
	chmod +x "$tmp"/*.sh
	if "$tmp/check-specs-test.sh" >/dev/null 2>&1; then
		echo "survived: $edit"
		survived=1
	else
		echo "killed:   $edit"
	fi
done <<'EOF'
s/README.adoc | constitution.adoc) continue/README.adoc | constitution.adoc | *.adoc) continue/
s/\^\[0-9\]{3}-/^[0-9]+-/
s/numbers+="$number "/:/
s/bad "$name" "no spec.adoc"/:/
s/draft | dropped) ;;/draft | dropped | *) ;;/
s/if ! grep -qx/if false \&\& ! grep -qx/
s/if \[\[ ! $value =~ $issue \]\]/if false/
s|1,/^\\$/s|s|
s/^exit $fail/exit 0/
EOF
exit $survived
