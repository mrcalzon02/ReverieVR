# ADR-0017 — Secure Diagnostic Issue Intake

**Status:** Accepted design baseline  
**Date:** 2026-10-05

## Context

ReverieVR already creates ZIP diagnostic bundles containing a build/device
manifest plus Standard or Development logs. The next useful step is one-tap
submission into the project's GitHub issue workflow.

The handset is an untrusted distribution surface. Anything compiled into the APK
must be assumed recoverable by anyone who obtains the APK. Therefore a GitHub
personal access token, GitHub App private key, repository secret, storage key, or
other privileged credential must never be packaged in the Android application.

GitHub's supported Issues API creates issues and comments, but it does not expose
a supported REST endpoint for programmatically uploading arbitrary ZIP files as
native issue attachments. Diagnostic bundles can also contain filenames, device
names, controller observations, timing data, and other information that should
not automatically become public.

## Decision

ReverieVR will use a split submission architecture.

### Handset

The APK may create the ZIP locally, generate a random diagnostic ID, calculate
SHA-256 and byte length, display a submission/privacy preview, allow cancellation
or redaction, POST the approved bundle to a configured HTTPS intake endpoint,
and receive the created GitHub issue URL.

The APK must not contain a GitHub PAT, GitHub App private key, storage access
key, Actions secret, or long-lived shared upload password.

### Intake broker

A small HTTPS broker receives the bundle. It must enforce request limits, verify
the ZIP, reject path traversal/malformed archives, rate-limit submissions,
recompute SHA-256, store the raw bundle privately, create the GitHub issue with a
least-privilege GitHub App installation token, and return the issue URL and
receipt ID.

Only sanitized metadata belongs in the public issue: diagnostic ID, timestamp,
app/build version, device/API summary, logging mode, user reproduction notes,
SHA-256, byte length, privacy status, and the controlled private bundle
reference.

### GitHub secrets and runtime secrets

GitHub Actions secrets are server-side workflow values. They are not an Android
credential-distribution mechanism.

If Actions deploys the intake broker, use an environment named
`diagnostic-intake`. Recommended deployment/runtime secret names are:

- `DIAGNOSTIC_GITHUB_APP_ID`
- `DIAGNOSTIC_GITHUB_APP_PRIVATE_KEY`
- `DIAGNOSTIC_STORAGE_ENDPOINT`
- `DIAGNOSTIC_STORAGE_ACCESS_KEY_ID`
- `DIAGNOSTIC_STORAGE_SECRET_ACCESS_KEY`
- `DIAGNOSTIC_STORAGE_BUCKET`

Where the hosting platform has its own secret store, runtime GitHub/storage
credentials should live there. GitHub Actions should hold only the minimum
credentials needed to deploy/update that service.

### Accepted first implementation

The first implementation uses a Cloudflare Worker in `diagnostic-intake/`:

- private R2 binding `DIAGNOSTIC_BUNDLES`;
- SQLite-backed Durable Object `RATE_LIMITER`;
- scheduled raw-bundle expiry after the configured retention window;
- GitHub App JWT/install-token exchange at runtime;
- public issue creation containing sanitized metadata only.

The protected GitHub environment `diagnostic-intake` supplies:

- `CLOUDFLARE_API_TOKEN`
- `CLOUDFLARE_ACCOUNT_ID`
- `DIAGNOSTIC_GITHUB_APP_ID`
- `DIAGNOSTIC_GITHUB_APP_PRIVATE_KEY`

The public endpoint is carried separately as the repository Actions variable
`DIAGNOSTIC_INTAKE_URL`; it is not a secret. The Android submit control is
hidden when this variable was not injected into the build.

The GitHub App needs only:

- Metadata: read
- Issues: read/write

It does not need repository Contents write access to file diagnostic issues.

### Bundle storage and retention

Raw bundles are private by default. A public repository release asset, commit,
gist, issue-body base64 blob, or undocumented issue attachment API is not an
acceptable automatic storage target.

Object keys are server-generated from the receipt/diagnostic ID, never from an
untrusted filename. Initial retention target is 30 days unless explicitly
preserved for an active investigation.

### Abuse and privacy controls

The intake endpoint must implement TLS, request/body size limits, rate limiting,
bounded upload time, ZIP entry-count and expanded-size limits, no arbitrary URL
fetching, no shell execution, no automatic publication of raw logs, server-side
hashing, and clear deletion/expiry behavior.

A future installation-registration or challenge mechanism may be added if a
public unauthenticated endpoint attracts abuse. It still must not ship a reusable
privileged secret in the APK.

## Failure semantics

Submission counts as successful only after the complete ZIP is durably stored,
the server-side hash is calculated, the GitHub issue is created, and the issue
number plus bundle receipt are returned to the handset.

If issue creation fails after storage succeeds, the receipt remains retryable
without requiring a second upload. If upload fails, ReverieVR preserves the local
bundle and the manual export path.

## Consequences

GitHub Issues remain the searchable triage record while sensitive, bulky
diagnostic ZIPs remain private.

The design adds a small server-side component, but it avoids turning every
installed APK into a repository credential and avoids reliance on undocumented
GitHub attachment behavior.
