# ReverieVR diagnostic intake broker

This Cloudflare Worker implements the server-side half of ADR-0017.

It accepts an explicitly approved diagnostic ZIP from the ReverieVR Android app,
rate-limits the sender, validates the ZIP central directory without extracting
it, recomputes SHA-256, stores the raw bundle in private R2 storage, creates a
sanitized GitHub Issue through a least-privilege GitHub App, and returns the
issue URL/receipt to the handset.

The raw ZIP is never attached to the public issue.

## Required Cloudflare resources

Create an R2 bucket named `reverievr-diagnostics`. `wrangler.toml` binds it as
`DIAGNOSTIC_BUNDLES`.

The Durable Object binding `RATE_LIMITER` provides a fixed-window limit per
hashed source IP. The scheduled Worker handler deletes raw bundle objects older
than `RETENTION_DAYS` (30 by default).

## GitHub App

Create a GitHub App installed only on the ReverieVR repository with:

- Metadata: read
- Issues: read/write

The Worker requires these runtime secrets:

- `DIAGNOSTIC_GITHUB_APP_ID`
- `DIAGNOSTIC_GITHUB_APP_PRIVATE_KEY`

The private key may be GitHub's normal `BEGIN RSA PRIVATE KEY` PEM or a PKCS#8
`BEGIN PRIVATE KEY` PEM.

## Deployment

The repository workflow `.github/workflows/deploy-diagnostic-intake.yml` uses
the protected GitHub environment `diagnostic-intake` and expects:

- `CLOUDFLARE_API_TOKEN`
- `CLOUDFLARE_ACCOUNT_ID`
- `DIAGNOSTIC_GITHUB_APP_ID`
- `DIAGNOSTIC_GITHUB_APP_PRIVATE_KEY`

The Cloudflare API token should be scoped only to the account/resources needed
to deploy this Worker, its Durable Object, and its R2 binding. Cloudflare's
current **Edit Cloudflare Workers** token template is an appropriate starting
point; narrow the account/zone resources to the account that hosts this intake
service rather than granting unrelated accounts. Do not use the Global API Key.

After deployment, set the repository Actions variable `DIAGNOSTIC_INTAKE_URL`
to the HTTPS endpoint, for example:

`https://reverievr-diagnostic-intake.<account-subdomain>.workers.dev/v1/diagnostics`

The Android build treats an empty value as "intake unavailable" and does not
show a dead Submit diagnostics control.

## Endpoint

`POST /v1/diagnostics` with `multipart/form-data` fields:

- `diagnostic_id`
- `privacy_reviewed=true`
- `bundle_sha256`
- `summary`
- `expected`
- `app_version`
- `build_type`
- `device`
- `android`
- `logging_mode`
- `bundle` (ZIP)

`GET /healthz` is credential-free and returns service health only.

### Stored-receipt finalize

If the ZIP is durably written to R2 but GitHub issue creation fails, the Worker
writes a private pending receipt before returning the error. The Android client
remembers only the diagnostic ID, SHA-256 and receipt reference.

It may then call:

`POST /v1/diagnostics/finalize`

with JSON:

```json
{
  "diagnostic_id": "revdiag-...",
  "bundle_sha256": "<64 hex characters>"
}
```

The Worker verifies the pending receipt and stored object, then retries GitHub
issue creation without accepting or transferring the ZIP a second time. The
diagnostic ID plus exact SHA-256 acts as proof of the already-stored receipt.
Completed and pending receipts are retained only for the configured retention
window.

## Retrieving a private diagnostic bundle

There is intentionally no public bundle-download endpoint.

For maintainer investigation, use the Cloudflare dashboard (or an authorized R2
administrative client) and open the private `reverievr-diagnostics` bucket.
Raw bundles use:

`diagnostics/YYYY/MM/<diagnostic-id>.zip`

The GitHub issue contains the diagnostic ID, submission timestamp and SHA-256,
so the object path can be located without publishing a storage credential.
Before inspecting a downloaded bundle, recompute SHA-256 and require it to match
the value recorded in the issue.

Private receipt records use:

`receipts/<diagnostic-id>.json`

Do not copy the raw ZIP into a public issue, release asset, gist, repository
commit or Actions log. Preserve a bundle beyond the normal retention window only
when an active investigation actually requires it.
