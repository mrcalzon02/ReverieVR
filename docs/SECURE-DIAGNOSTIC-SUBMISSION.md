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

## Secrets

Do not place runtime credentials in Gradle properties, BuildConfig fields,
resources, source code, assets, native libraries, or generated APK metadata.

For an Actions-driven deployment, create a protected environment named
`diagnostic-intake`. The intended secret names and permission boundaries are
defined in ADR-0017.

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
