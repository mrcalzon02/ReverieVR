param(
    [switch]$SetGitHubSecrets,
    [switch]$TriggerBuild,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$Repo = "mrcalzon02/ReverieVR"
$Alias = "reverievr-phone-test"
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$OutputDir = Join-Path $ProjectRoot ".local\reverievr-signing"
$Keystore = Join-Path $OutputDir "reverievr-phone-test.jks"
$Certificate = Join-Path $OutputDir "reverievr-phone-test-cert.der"
$SecretsFile = Join-Path $OutputDir "github-secrets.txt"
$FingerprintFile = Join-Path $OutputDir "certificate-sha256.txt"

function New-RandomSecret {
    $bytes = New-Object byte[] 32
    [System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
    return [Convert]::ToBase64String($bytes).Replace("+", "-").Replace("/", "_").TrimEnd("=")
}

$keytool = Get-Command keytool -ErrorAction SilentlyContinue
if (-not $keytool) {
    throw "keytool was not found. Install/use JDK 17 and make sure keytool is on PATH."
}

if ((Test-Path $Keystore) -and -not $Force) {
    throw "Signing keystore already exists at $Keystore. Refusing to replace the permanent identity. Use -Force only if you intentionally want a new signing lineage."
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$StorePassword = New-RandomSecret
$KeyPassword = New-RandomSecret

if (Test-Path $Keystore) {
    Remove-Item -Force $Keystore
}
if (Test-Path $Certificate) {
    Remove-Item -Force $Certificate
}

$genArgs = @(
    "-genkeypair",
    "-alias", $Alias,
    "-keyalg", "RSA",
    "-keysize", "3072",
    "-sigalg", "SHA256withRSA",
    "-validity", "36500",
    "-storetype", "JKS",
    "-keystore", $Keystore,
    "-storepass", $StorePassword,
    "-keypass", $KeyPassword,
    "-dname", "CN=ReverieVR Distribution, O=ReverieVR, C=US",
    "-noprompt"
)
& $keytool.Source @genArgs
if ($LASTEXITCODE -ne 0) {
    throw "keytool failed to create the ReverieVR signing keystore."
}

$exportArgs = @(
    "-exportcert",
    "-alias", $Alias,
    "-keystore", $Keystore,
    "-storepass", $StorePassword,
    "-file", $Certificate
)
& $keytool.Source @exportArgs
if ($LASTEXITCODE -ne 0) {
    throw "keytool failed to export the public signing certificate."
}

$Fingerprint = (Get-FileHash -Algorithm SHA256 -Path $Certificate).Hash.ToLowerInvariant()
if ($Fingerprint -notmatch "^[0-9a-f]{64}$") {
    throw "Could not calculate a valid certificate SHA-256 fingerprint."
}

$KeystoreBase64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes($Keystore))

$SecretsText = @"
PHONE_TEST_KEYSTORE_BASE64=$KeystoreBase64
PHONE_TEST_STORE_PASSWORD=$StorePassword
PHONE_TEST_KEY_ALIAS=$Alias
PHONE_TEST_KEY_PASSWORD=$KeyPassword
"@

[IO.File]::WriteAllText($SecretsFile, $SecretsText, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText($FingerprintFile, $Fingerprint + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))

if ($TriggerBuild -and -not $SetGitHubSecrets) {
    throw "-TriggerBuild requires -SetGitHubSecrets so the workflow cannot start without the permanent signer."
}

if ($SetGitHubSecrets) {
    $gh = Get-Command gh -ErrorAction SilentlyContinue
    if (-not $gh) {
        throw "GitHub CLI (gh) is not installed. The keystore was created successfully; add the four values from $SecretsFile manually in GitHub Actions secrets."
    }

    & $gh.Source auth status | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "GitHub CLI is not authenticated. Run 'gh auth login', then rerun this script with -SetGitHubSecrets."
    }

    $secretValues = @{
        PHONE_TEST_KEYSTORE_BASE64 = $KeystoreBase64
        PHONE_TEST_STORE_PASSWORD = $StorePassword
        PHONE_TEST_KEY_ALIAS = $Alias
        PHONE_TEST_KEY_PASSWORD = $KeyPassword
    }

    foreach ($entry in $secretValues.GetEnumerator()) {
        $entry.Value | & $gh.Source secret set $entry.Key --repo $Repo
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to set GitHub Actions secret $($entry.Key)."
        }
    }

    Write-Host "GitHub Actions signing secrets were populated for $Repo."

    if ($TriggerBuild) {
        & $gh.Source workflow run phone-test-release.yml --repo $Repo
        if ($LASTEXITCODE -ne 0) {
            throw "Signing secrets were saved, but the phone-test workflow could not be triggered."
        }
        Write-Host "First persistently signed phone-test build was triggered."
    }
}

Write-Host ""
Write-Host "Permanent ReverieVR signing identity created."
Write-Host "Keystore: $Keystore"
Write-Host "Secret values: $SecretsFile"
Write-Host "Certificate SHA-256: $Fingerprint"
Write-Host ""
Write-Host "BACK UP THE KEYSTORE AND github-secrets.txt IN TWO TRUSTED OFFLINE LOCATIONS."
Write-Host "Do not commit either file. Losing this keystore prevents seamless future updates."
if (-not $SetGitHubSecrets) {
    Write-Host ""
    Write-Host "Next: add the four values from github-secrets.txt as GitHub Actions repository secrets,"
    Write-Host "or install/authenticate GitHub CLI and rerun with -SetGitHubSecrets."
}
