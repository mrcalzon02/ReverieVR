#!/usr/bin/env sh
set -eu
# Author-hosted original Jetpack 1.5 + Squarez Deluxe freeware ZIP.
# Keep archive unchanged, SHA-pin the version currently hosted by Adept.
URL="https://www.adeptsoftware.com/jetpack/jetpak15.zip"
EXPECTED_BYTES=494745
EXPECTED_MD5="52d5bc3656f28cfba23029aceffcc95f"
EXPECTED_SHA256="03f4979149bdeeb43091517e0f33f92b14cc2e5d7531e0c524749c3383fc9d4c"
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
if [ "$actual_md5" != "$EXPECTED_MD5" ]; then
    echo "Jetpack author archive MD5 mismatch; refusing changed original ZIP." >&2
    exit 1
fi
if command -v sha256sum >/dev/null 2>&1; then
    actual_sha="$(sha256sum "$TMP" | awk '{print $1}')"
else
    actual_sha="$(shasum -a 256 "$TMP" | awk '{print $1}')"
fi
if [ "$actual_sha" != "$EXPECTED_SHA256" ]; then
    echo "Jetpack author archive SHA-256 mismatch; refusing changed original ZIP." >&2
    exit 1
fi
unzip -tq "$TMP" >/dev/null
entries="$(unzip -Z -1 "$TMP")"
printf '%s\n' "$entries" | grep -Fqx 'jetpak15/JETPACK.EXE' || {
    echo "Original Jetpack archive missing JETPACK.EXE." >&2; exit 1;
}
printf '%s\n' "$entries" | grep -Fqx 'jetpak15/SQUAREZ/SQUAREZ.EXE' || {
    echo "Original Squarez archive missing SQUAREZ.EXE." >&2; exit 1;
}
mv "$TMP" "$DEST"
echo "Verified Adept original Jetpack+Squarez ZIP: $EXPECTED_SHA256"
