#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
export PATH="$scratch:$PATH"

printf '#!/bin/sh\nexit 1\n' > "$scratch/arduino-cli"
chmod +x "$scratch/arduino-cli"
if bash tools/upload_both.sh > "$scratch/output" 2>&1; then exit 1; fi
grep -q 'Failed to discover USB ports' "$scratch/output"
if grep -q 'found 0' "$scratch/output"; then exit 1; fi

printf '#!/bin/sh\nprintf '\''{"detected_ports":[]}\\n'\''\n' > "$scratch/arduino-cli"
if bash tools/upload_both.sh > "$scratch/output" 2>&1; then exit 1; fi
grep -q 'found 0' "$scratch/output"
