#!/bin/sh
set -eu

EXPECTED_WRAPPER_SHA256="497c8c2a7e5031f6aa847f88104aa80a93532ec32ee17bdb8d1d2f67a194a9c7"
EXPECTED_DISTRIBUTION_SHA256="9c0f7faeeb306cb14e4279a3e084ca6b596894089a0638e68a07c945a32c9e14"
EXPECTED_CARDBOARD_SHA="5969239e7c87f4cd64c8ec170ce1e7f4eb559e37"
EXPECTED_DOSBOX_PURE_SHA="a4a0bab7f8931433588f2fcad9045c85b277373d"

hash_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    else
        echo "No SHA-256 tool found (sha256sum or shasum required)." >&2
        exit 2
    fi
}

wrapper_hash=$(hash_file gradle/wrapper/gradle-wrapper.jar)
if [ "$wrapper_hash" != "$EXPECTED_WRAPPER_SHA256" ]; then
    echo "Gradle wrapper JAR checksum mismatch: $wrapper_hash" >&2
    exit 3
fi

if ! grep -Fq "distributionSha256Sum=$EXPECTED_DISTRIBUTION_SHA256" gradle/wrapper/gradle-wrapper.properties; then
    echo "Gradle distribution checksum is missing or wrong." >&2
    exit 4
fi

if [ ! -d third_party/cardboard/.git ] && [ ! -f third_party/cardboard/.git ]; then
    echo "Cardboard submodule is not initialized." >&2
    echo "Run: git submodule update --init --recursive" >&2
    exit 5
fi

cardboard_hash=$(git -C third_party/cardboard rev-parse HEAD)
if [ "$cardboard_hash" != "$EXPECTED_CARDBOARD_SHA" ]; then
    echo "Cardboard submodule mismatch: $cardboard_hash" >&2
    exit 6
fi

dosbox_status="not fetched (native DOS runtime disabled)"
if [ -d third_party/dosbox-pure/src/.git ] || [ -f third_party/dosbox-pure/src/.git ]; then
    dosbox_hash=$(git -C third_party/dosbox-pure/src rev-parse HEAD)
    if [ "$dosbox_hash" != "$EXPECTED_DOSBOX_PURE_SHA" ]; then
        echo "DOSBox Pure checkout mismatch: $dosbox_hash" >&2
        exit 7
    fi
    dosbox_status="$dosbox_hash"
fi

echo "ReverieVR build bootstrap verified."
echo "  Gradle wrapper: $wrapper_hash"
echo "  Gradle distribution SHA-256: $EXPECTED_DISTRIBUTION_SHA256"
echo "  Cardboard: $cardboard_hash"
echo "  DOSBox Pure: $dosbox_status"
