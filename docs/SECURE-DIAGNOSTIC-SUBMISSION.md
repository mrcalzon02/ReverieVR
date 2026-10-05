# Secure Diagnostic Submission

ReverieVR diagnostic submission is split between the Android app, a small HTTPS
intake service, private bundle storage, and GitHub Issues.

The Android app is never given a privileged GitHub credential. GitHub repository
or environment secrets are available to Actions workflows only; they are not a
safe way to inject credentials into an APK.

## Target flow

1. User enables Standard or Development logging and reproduces the fault.
2. ReverieVR builds the same ZIP available through manual Export diagnostics.
3. ReverieVR shows a submission preview/privacy warning.
4. User explicitly submits.
5. The app uploads the ZIP to the HTTPS intake endpoint.
6. The intake service validates, hashes, rate-limits, and privately stores it.
7. The intake service creates a GitHub issue with sanitized metadata and the
   private receipt/reference.
8. The app displays the issue URL.
9. If any network step fails, local/manual export remains available.

## Secrets and deployment values

Do not place runtime credentials in Gradle properties, BuildConfig fields,
resources, source code, assets, native libraries, or generated APK metadata.

The first broker implementation lives in `diagnostic-intake/` and targets a
Cloudflare Worker with private R2 storage plus a Durable Object rate limiter.

Create/use a protected GitHub environment named `diagnostic-intake` with these
environment secrets:

- `CLOUDFLARE_API_TOKEN`
- `CLOUDFLARE_ACCOUNT_ID`
- `DIAGNOSTIC_GITHUB_APP_ID`
- `DIAGNOSTIC_GITHUB_APP_PRIVATE_KEY`

The repository workflow
`.github/workflows/deploy-diagnostic-intake.yml` deploys the Worker and copies
only the GitHub App values into the Worker's runtime secret store. The Android
APK never receives those values.

After the Worker is deployed, set the non-secret GitHub Actions repository
variable `DIAGNOSTIC_INTAKE_URL` to its HTTPS
`/v1/diagnostics` endpoint. Phone-test builds pass that public URL into
`BuildConfig.DIAGNOSTIC_INTAKE_URL`. When the variable is absent or invalid,
ReverieVR hides the Submit diagnostics button rather than exposing a dead
control.

The R2 bucket `reverievr-diagnostics` must exist before deployment. Runtime R2
access is through the Worker binding, so no R2 access key is compiled into the
Worker or APK.

Where possible, runtime secrets belong in the hosting platform's own secret
store. GitHub Actions secrets should be used to deploy or rotate them, not copied
into application artifacts.

## GitHub App

The preferred broker credential is a GitHub App installation credential with
only:

- Metadata: read
- Issues: read/write

A broad personal access token is not the preferred long-term credential.

## Bundle privacy

The raw ZIP is private by default. The public issue gets the receipt ID and
SHA-256, not the ZIP contents.

Manual export remains permanently supported because it is useful when the
network path itself is broken.
