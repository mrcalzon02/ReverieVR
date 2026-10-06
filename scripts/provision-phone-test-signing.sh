#!/usr/bin/env bash
set -euo pipefail

REPO="mrcalzon02/ReverieVR"
ALIAS="reverievr-phone-test"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="$PROJECT_ROOT/.local/reverievr-signing"
KEYSTORE="$OUTPUT_DIR/reverievr-phone-test.jks"
CERTIFICATE="$OUTPUT_DIR/reverievr-phone-test-cert.der"
SECRETS_FILE="$OUTPUT_DIR/github-secrets.txt"
FINGERPRINT_FILE="$OUTPUT_DIR/certificate-sha256.txt"

SET_GITHUB=false
TRIGGER_BUILD=false
FORCE=false
for arg in "$@"; do
  case "$arg" in
    --set-github-secrets) SET_GITHUB=true ;;
    --trigger-build) TRIGGER_BUILD=true ;;
    --force) FORCE=true ;;
    *)
      echo "Unknown argument: $arg" >&2
      exit 2
      ;;
  esac
done

command -v keytool >/dev/null || {
  echo "keytool was not found. Install/use JDK 17." >&2
  exit 1
}
command -v python3 >/dev/null || {
  echo "python3 is required to generate strong local passwords." >&2
  exit 1
}

if [[ -e "$KEYSTORE" && "$FORCE" != true ]]; then
  echo "Signing keystore already exists at $KEYSTORE." >&2
  echo "Refusing to replace the permanent signing identity. Use --force only intentionally." >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"
chmod 700 "$OUTPUT_DIR" || true

random_secret() {
  python3 - <<'PY'
import base64, secrets
print(base64.urlsafe_b64encode(secrets.token_bytes(32)).decode().rstrip("="))
PY
}

STORE_PASSWORD="$(random_secret)"
KEY_PASSWORD="$(random_secret)"

rm -f "$KEYSTORE" "$CERTIFICATE"

keytool \
  -genkeypair \
  -alias "$ALIAS" \
  -keyalg RSA \
  -keysize 3072 \
  -sigalg SHA256withRSA \
  -validity 36500 \
  -storetype JKS \
  -keystore "$KEYSTORE" \
  -storepass "$STORE_PASSWORD" \
  -keypass "$KEY_PASSWORD" \
  -dname "CN=ReverieVR Distribution, O=ReverieVR, C=US" \
  -noprompt >/dev/null

keytool \
  -exportcert \
  -alias "$ALIAS" \
  -keystore "$KEYSTORE" \
  -storepass "$STORE_PASSWORD" \
  -file "$CERTIFICATE" >/dev/null

if command -v sha256sum >/dev/null; then
  FINGERPRINT="$(sha256sum "$CERTIFICATE" | awk '{print $1}')"
elif command -v shasum >/dev/null; then
  FINGERPRINT="$(shasum -a 256 "$CERTIFICATE" | awk '{print $1}')"
else
  echo "Neither sha256sum nor shasum is available." >&2
  exit 1
fi
[[ "$FINGERPRINT" =~ ^[0-9a-f]{64}$ ]]

KEYSTORE_BASE64="$(base64 < "$KEYSTORE" | tr -d '\r\n')"

cat > "$SECRETS_FILE" <<EOF
PHONE_TEST_KEYSTORE_BASE64=$KEYSTORE_BASE64
PHONE_TEST_STORE_PASSWORD=$STORE_PASSWORD
PHONE_TEST_KEY_ALIAS=$ALIAS
PHONE_TEST_KEY_PASSWORD=$KEY_PASSWORD
EOF
printf '%s\n' "$FINGERPRINT" > "$FINGERPRINT_FILE"
chmod 600 "$KEYSTORE" "$SECRETS_FILE" "$FINGERPRINT_FILE" "$CERTIFICATE" || true

if [[ "$TRIGGER_BUILD" == true && "$SET_GITHUB" != true ]]; then
  echo "--trigger-build requires --set-github-secrets." >&2
  exit 1
fi

if [[ "$SET_GITHUB" == true ]]; then
  command -v gh >/dev/null || {
    echo "GitHub CLI is not installed. Files were generated locally." >&2
    exit 1
  }
  gh auth status >/dev/null

  printf '%s' "$KEYSTORE_BASE64" | gh secret set PHONE_TEST_KEYSTORE_BASE64 --repo "$REPO"
  printf '%s' "$STORE_PASSWORD" | gh secret set PHONE_TEST_STORE_PASSWORD --repo "$REPO"
  printf '%s' "$ALIAS" | gh secret set PHONE_TEST_KEY_ALIAS --repo "$REPO"
  printf '%s' "$KEY_PASSWORD" | gh secret set PHONE_TEST_KEY_PASSWORD --repo "$REPO"

  echo "GitHub Actions signing secrets were populated for $REPO."

  if [[ "$TRIGGER_BUILD" == true ]]; then
    gh workflow run phone-test-release.yml --repo "$REPO"
    echo "First persistently signed phone-test build was triggered."
  fi
fi

echo
echo "Permanent ReverieVR signing identity created."
echo "Keystore: $KEYSTORE"
echo "Secret values: $SECRETS_FILE"
echo "Certificate SHA-256: $FINGERPRINT"
echo
echo "BACK UP THE KEYSTORE AND github-secrets.txt IN TWO TRUSTED OFFLINE LOCATIONS."
echo "Do not commit either file. Losing this keystore prevents seamless future updates."
if [[ "$SET_GITHUB" != true ]]; then
  echo
  echo "Next: add the four values from github-secrets.txt as GitHub Actions repository secrets,"
  echo "or authenticate GitHub CLI and rerun with --set-github-secrets."
fi
