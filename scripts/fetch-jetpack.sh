#!/usr/bin/env sh
set -eu
# The complete author-hosted Jetpack 1.5 + Squarez Deluxe freeware ZIP,
# original 1998 release from Adept Software, intentionally kept unchanged.
URL="https://www.adeptsoftware.com/jetpack/jetpak15.zip"
# The author's currently hosted file is a distinct revision from the
# preserved 1998 mirror (495141 B / MD5 8ae51...). Do not assume equality.
EXPECTED_BYTES=494745
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
DEST="$ROOT/third_party/jetpack/payload/default-content/jetpak15.zip"
TMP="$DEST.tmp"
mkdir -p "$(dirname "$DEST")"
trap 'rm -f "$TMP"' EXIT
curl --fail --location --proto '=https' --tlsv1.2 \
    --connect-timeout 20 --max-time 90 "$URL" --output "$TMP"
actual_bytes="$(wc -c < "$TMP" | tr -d ' ')"
if [ "$actual_bytes" != "$EXPECTED_BYTES" ]; then
    echo "Jetpack archive size mismatch: expected $EXPECTED_BYTES, got $actual_bytes" >&2
    exit 1
fi
if command -v md5sum >/dev/null 2>&1; then
    actual_md5="$(md5sum "$TMP" | awk '{print $1}')"
else
    actual_md5="$(md5 -q "$TMP")"
fi
echo "Author-hosted Jetpack current archive MD5: $actual_md5"
unzip -tq "$TMP" >/dev/null
echo "Author-hosted Jetpack archive entries:"
unzip -Z -1 "$TMP"
mv "$TMP" "$DEST"
sha256sum "$DEST"
echo "Verified ZIP integrity and official author-hosted Jetpack package; SHA pin pending."
