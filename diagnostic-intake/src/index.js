const JSON_HEADERS = {
  "content-type": "application/json; charset=utf-8",
  "cache-control": "no-store"
};

const GITHUB_API = "https://api.github.com";
const USER_AGENT = "ReverieVR-Diagnostic-Intake/1";
const RECEIPT_PREFIX = "receipts/";
const BUNDLE_PREFIX = "diagnostics/";

export default {
  async fetch(request, env) {
    try {
      const url = new URL(request.url);
      if (request.method === "GET" && url.pathname === "/healthz") {
        return json({ok: true, service: "reverievr-diagnostic-intake"}, 200);
      }

      if (request.method !== "POST" || url.pathname !== "/v1/diagnostics") {
        return json({error: "not_found"}, 404);
      }

      return await handleDiagnosticSubmission(request, env);
    } catch (error) {
      console.error("Unhandled intake error", error);
      return json({error: "internal_error"}, 500);
    }
  },

  async scheduled(controller, env, ctx) {
    ctx.waitUntil(expireOldBundles(env));
  }
};

export class DiagnosticRateLimiter {
  constructor(state, env) {
    this.state = state;
    this.env = env;
  }

  async fetch(request) {
    if (request.method !== "POST") {
      return json({error: "method_not_allowed"}, 405);
    }

    const limit = positiveInt(this.env.RATE_LIMIT_PER_HOUR, 6);
    const now = Date.now();
    const hour = 60 * 60 * 1000;
    let record = await this.state.storage.get("window");

    if (!record || now - record.startedAt >= hour) {
      record = {startedAt: now, count: 0};
    }

    if (record.count >= limit) {
      const retryAfterSeconds = Math.max(
        1,
        Math.ceil((record.startedAt + hour - now) / 1000)
      );
      return new Response(
        JSON.stringify({error: "rate_limited", retryAfterSeconds}),
        {
          status: 429,
          headers: {
            ...JSON_HEADERS,
            "retry-after": String(retryAfterSeconds)
          }
        }
      );
    }

    record.count += 1;
    await this.state.storage.put("window", record);
    return json({ok: true, remaining: Math.max(0, limit - record.count)}, 200);
  }
}

async function handleDiagnosticSubmission(request, env) {
  requireRuntimeBindings(env);

  const contentLength = Number(request.headers.get("content-length") || "0");
  const maxBundleBytes = positiveInt(env.MAX_BUNDLE_BYTES, 25 * 1024 * 1024);
  const maxRequestBytes = maxBundleBytes + 512 * 1024;
  if (contentLength > 0 && contentLength > maxRequestBytes) {
    return json({error: "request_too_large"}, 413);
  }

  const rateResponse = await enforceRateLimit(request, env);
  if (!rateResponse.ok) {
    return rateResponse;
  }

  const contentType = request.headers.get("content-type") || "";
  if (!contentType.toLowerCase().startsWith("multipart/form-data")) {
    return json({error: "multipart_required"}, 415);
  }

  const boundedBody = await readBodyBounded(request, maxRequestBytes);
  if (!boundedBody.ok) {
    return json(
      {error: boundedBody.error},
      boundedBody.status
    );
  }

  let form;
  try {
    form = await new Response(
      boundedBody.bytes,
      {headers: {"content-type": contentType}}
    ).formData();
  } catch (error) {
    return json({error: "invalid_multipart"}, 400);
  }

  const diagnosticId = textField(form, "diagnostic_id", 80);
  if (!/^revdiag-[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i.test(diagnosticId)) {
    return json({error: "invalid_diagnostic_id"}, 400);
  }

  if (textField(form, "privacy_reviewed", 16).toLowerCase() !== "true") {
    return json({error: "privacy_review_required"}, 400);
  }

  const receiptKey = `${RECEIPT_PREFIX}${diagnosticId}.json`;
  const previousReceipt = await readReceipt(env.DIAGNOSTIC_BUNDLES, receiptKey);
  if (previousReceipt && previousReceipt.issueUrl) {
    return json(previousReceipt, 200);
  }

  const bundle = form.get("bundle");
  if (!(bundle instanceof File)) {
    return json({error: "bundle_required"}, 400);
  }
  if (bundle.size <= 0 || bundle.size > maxBundleBytes) {
    return json({error: "bundle_size_invalid", maxBytes: maxBundleBytes}, 413);
  }

  const bytes = new Uint8Array(await bundle.arrayBuffer());
  const zipCheck = inspectZip(bytes, {
    maxEntries: positiveInt(env.MAX_ZIP_ENTRIES, 128),
    maxExpandedBytes: positiveInt(env.MAX_EXPANDED_BYTES, 128 * 1024 * 1024)
  });
  if (!zipCheck.ok) {
    return json({error: "invalid_bundle", reason: zipCheck.reason}, 400);
  }

  const sha256 = await sha256Hex(bytes);
  const clientSha = textField(form, "bundle_sha256", 80).toLowerCase();
  if (clientSha && clientSha !== sha256) {
    return json({error: "bundle_hash_mismatch"}, 400);
  }

  const now = new Date();
  const metadata = {
    diagnosticId,
    sha256,
    byteLength: bytes.byteLength,
    createdAt: now.toISOString(),
    appVersion: textField(form, "app_version", 120),
    buildType: textField(form, "build_type", 80),
    device: textField(form, "device", 180),
    android: textField(form, "android", 120),
    loggingMode: textField(form, "logging_mode", 40),
    summary: textField(form, "summary", 2000),
    expected: textField(form, "expected", 2000),
    receiptReference: diagnosticId
  };

  if (!metadata.summary) {
    return json({error: "summary_required"}, 400);
  }

  const year = String(now.getUTCFullYear());
  const month = String(now.getUTCMonth() + 1).padStart(2, "0");
  const objectKey = `${BUNDLE_PREFIX}${year}/${month}/${diagnosticId}.zip`;

  const existing = await env.DIAGNOSTIC_BUNDLES.head(objectKey);
  if (existing) {
    const previousHash = existing.customMetadata?.sha256 || "";
    if (previousHash && previousHash !== sha256) {
      return json({error: "diagnostic_id_collision"}, 409);
    }
  } else {
    await env.DIAGNOSTIC_BUNDLES.put(objectKey, bytes, {
      httpMetadata: {contentType: "application/zip"},
      customMetadata: {
        diagnosticId,
        sha256,
        createdAt: now.toISOString()
      }
    });
  }

  let issue;
  try {
    issue = await createGitHubIssue(env, metadata);
  } catch (error) {
    console.error("Issue creation failed", error);
    return json({
      error: "issue_creation_failed",
      diagnosticId,
      receiptReference: diagnosticId,
      sha256,
      stored: true
    }, 502);
  }

  const receipt = {
    ok: true,
    diagnosticId,
    receiptReference: diagnosticId,
    sha256,
    byteLength: bytes.byteLength,
    issueNumber: issue.number,
    issueUrl: issue.html_url
  };

  await env.DIAGNOSTIC_BUNDLES.put(
    receiptKey,
    JSON.stringify(receipt),
    {httpMetadata: {contentType: "application/json"}}
  );

  return json(receipt, 201);
}

function requireRuntimeBindings(env) {
  if (!env.DIAGNOSTIC_BUNDLES || !env.RATE_LIMITER) {
    throw new Error("Diagnostic storage/rate-limit bindings are missing.");
  }
  if (!env.DIAGNOSTIC_GITHUB_APP_ID || !env.DIAGNOSTIC_GITHUB_APP_PRIVATE_KEY) {
    throw new Error("GitHub App runtime secrets are missing.");
  }
  if (!env.GITHUB_REPOSITORY || !/^[^/]+\/[^/]+$/.test(env.GITHUB_REPOSITORY)) {
    throw new Error("GITHUB_REPOSITORY is invalid.");
  }
}

async function enforceRateLimit(request, env) {
  const address = request.headers.get("cf-connecting-ip") || "unknown";
  const digest = await sha256Hex(new TextEncoder().encode(address));
  const id = env.RATE_LIMITER.idFromName(digest);
  const stub = env.RATE_LIMITER.get(id);
  return await stub.fetch("https://rate-limit.local/check", {method: "POST"});
}

async function readBodyBounded(request, maxBytes) {
  if (!request.body) {
    return {
      ok: false,
      error: "request_body_required",
      status: 400
    };
  }

  const reader = request.body.getReader();
  const chunks = [];
  let total = 0;

  try {
    while (true) {
      const part = await reader.read();
      if (part.done) {
        break;
      }

      const value = part.value;
      if (!value || value.byteLength === 0) {
        continue;
      }

      total += value.byteLength;
      if (total > maxBytes) {
        await reader.cancel("request_too_large");
        return {
          ok: false,
          error: "request_too_large",
          status: 413
        };
      }

      chunks.push(value);
    }
  } catch (error) {
    return {
      ok: false,
      error: "request_body_read_failed",
      status: 400
    };
  }

  if (total === 0) {
    return {
      ok: false,
      error: "request_body_required",
      status: 400
    };
  }

  const bytes = new Uint8Array(total);
  let offset = 0;
  for (const chunk of chunks) {
    bytes.set(chunk, offset);
    offset += chunk.byteLength;
  }

  return {ok: true, bytes};
}

function textField(form, name, maxLength) {
  const value = form.get(name);
  if (typeof value !== "string") {
    return "";
  }
  return value
    .replace(/[\u0000-\u0008\u000B\u000C\u000E-\u001F\u007F]/g, "")
    .trim()
    .slice(0, maxLength);
}

function inspectZip(bytes, limits) {
  if (bytes.length < 22 || readU32(bytes, 0) !== 0x04034b50) {
    return {ok: false, reason: "missing_zip_header"};
  }

  const searchStart = Math.max(0, bytes.length - 65557);
  let eocd = -1;
  for (let index = bytes.length - 22; index >= searchStart; index--) {
    if (readU32(bytes, index) === 0x06054b50) {
      eocd = index;
      break;
    }
  }
  if (eocd < 0) {
    return {ok: false, reason: "missing_central_directory"};
  }

  const diskEntries = readU16(bytes, eocd + 8);
  const totalEntries = readU16(bytes, eocd + 10);
  const directorySize = readU32(bytes, eocd + 12);
  const directoryOffset = readU32(bytes, eocd + 16);
  const archiveCommentLength = readU16(bytes, eocd + 20);

  if (eocd + 22 + archiveCommentLength !== bytes.length) {
    return {ok: false, reason: "trailing_zip_data"};
  }

  if (diskEntries === 0xffff || totalEntries === 0xffff ||
      directorySize === 0xffffffff || directoryOffset === 0xffffffff) {
    return {ok: false, reason: "zip64_not_supported"};
  }
  if (diskEntries !== totalEntries) {
    return {ok: false, reason: "multi_disk_zip_not_supported"};
  }
  if (totalEntries <= 0 || totalEntries > limits.maxEntries) {
    return {ok: false, reason: "zip_entry_count_invalid"};
  }
  if (directoryOffset + directorySize !== eocd || directoryOffset < 0) {
    return {ok: false, reason: "central_directory_bounds_invalid"};
  }

  const decoder = new TextDecoder("utf-8", {fatal: false});
  let cursor = directoryOffset;
  let expanded = 0;
  let manifestSeen = false;

  for (let entry = 0; entry < totalEntries; entry++) {
    if (cursor + 46 > bytes.length || readU32(bytes, cursor) !== 0x02014b50) {
      return {ok: false, reason: "central_directory_entry_invalid"};
    }

    const flags = readU16(bytes, cursor + 8);
    const compressedSize = readU32(bytes, cursor + 20);
    const uncompressedSize = readU32(bytes, cursor + 24);
    const nameLength = readU16(bytes, cursor + 28);
    const extraLength = readU16(bytes, cursor + 30);
    const commentLength = readU16(bytes, cursor + 32);
    const localHeaderOffset = readU32(bytes, cursor + 42);

    if ((flags & 0x0001) !== 0) {
      return {ok: false, reason: "encrypted_zip_not_supported"};
    }
    if (compressedSize === 0xffffffff || uncompressedSize === 0xffffffff) {
      return {ok: false, reason: "zip64_not_supported"};
    }

    const nameStart = cursor + 46;
    const nameEnd = nameStart + nameLength;
    if (nameEnd > bytes.length) {
      return {ok: false, reason: "zip_filename_bounds_invalid"};
    }

    const name = decoder.decode(bytes.slice(nameStart, nameEnd)).replace(/\\/g, "/");
    if (!safeZipPath(name)) {
      return {ok: false, reason: "unsafe_zip_path"};
    }

    if (localHeaderOffset + 30 > directoryOffset
        || readU32(bytes, localHeaderOffset) !== 0x04034b50) {
      return {ok: false, reason: "local_header_invalid"};
    }

    const localNameLength = readU16(bytes, localHeaderOffset + 26);
    const localExtraLength = readU16(bytes, localHeaderOffset + 28);
    const localNameStart = localHeaderOffset + 30;
    const localNameEnd = localNameStart + localNameLength;
    const localDataStart = localNameEnd + localExtraLength;
    if (localDataStart > directoryOffset
        || localDataStart + compressedSize > directoryOffset) {
      return {ok: false, reason: "local_entry_bounds_invalid"};
    }

    const localName =
      decoder.decode(bytes.slice(localNameStart, localNameEnd)).replace(/\\/g, "/");
    if (localName !== name) {
      return {ok: false, reason: "local_name_mismatch"};
    }

    if (name === "manifest.txt") {
      manifestSeen = true;
    } else if (!/^logs\/reverie-[^/]+$/.test(name)) {
      return {ok: false, reason: "unexpected_zip_entry"};
    }

    expanded += uncompressedSize;
    if (expanded > limits.maxExpandedBytes) {
      return {ok: false, reason: "zip_expanded_size_exceeded"};
    }

    cursor = nameEnd + extraLength + commentLength;
  }

  if (cursor !== directoryOffset + directorySize) {
    return {ok: false, reason: "central_directory_size_mismatch"};
  }
  if (!manifestSeen) {
    return {ok: false, reason: "missing_manifest"};
  }

  return {ok: true, entries: totalEntries, expandedBytes: expanded};
}

function safeZipPath(name) {
  if (!name || name.includes("\u0000") || name.startsWith("/") || /^[A-Za-z]:/.test(name)) {
    return false;
  }
  const parts = name.split("/");
  return !parts.some(part => part === "..");
}

function readU16(bytes, offset) {
  return bytes[offset] | (bytes[offset + 1] << 8);
}

function readU32(bytes, offset) {
  return (
    bytes[offset] |
    (bytes[offset + 1] << 8) |
    (bytes[offset + 2] << 16) |
    (bytes[offset + 3] << 24)
  ) >>> 0;
}

async function createGitHubIssue(env, metadata) {
  const appJwt = await createGitHubAppJwt(
    env.DIAGNOSTIC_GITHUB_APP_ID,
    env.DIAGNOSTIC_GITHUB_APP_PRIVATE_KEY
  );

  const installation = await githubJson(
    `${GITHUB_API}/repos/${env.GITHUB_REPOSITORY}/installation`,
    {
      headers: githubHeaders(appJwt)
    }
  );

  const tokenResponse = await githubJson(
    `${GITHUB_API}/app/installations/${installation.id}/access_tokens`,
    {
      method: "POST",
      headers: githubHeaders(appJwt)
    }
  );

  const existingIssue = await findExistingDiagnosticIssue(
    env,
    tokenResponse.token,
    metadata.diagnosticId
  );
  if (existingIssue) {
    return existingIssue;
  }

  const titleSummary = publicText(metadata.summary, 90);
  const title = `[Diagnostic] ${metadata.diagnosticId} — ${titleSummary || "Runtime report"}`;
  const body = buildIssueBody(metadata);

  return await githubJson(
    `${GITHUB_API}/repos/${env.GITHUB_REPOSITORY}/issues`,
    {
      method: "POST",
      headers: githubHeaders(tokenResponse.token),
      body: JSON.stringify({title, body})
    }
  );
}

async function findExistingDiagnosticIssue(
  env,
  token,
  diagnosticId
) {
  const marker = `[Diagnostic] ${diagnosticId}`;

  for (let page = 1; page <= 3; page++) {
    const issues = await githubJson(
      `${GITHUB_API}/repos/${env.GITHUB_REPOSITORY}/issues`
        + `?state=all&sort=created&direction=desc&per_page=100&page=${page}`,
      {
        headers: githubHeaders(token)
      }
    );

    if (!Array.isArray(issues)) {
      return null;
    }

    const match = issues.find(
      issue =>
        !issue.pull_request
          && typeof issue.title === "string"
          && issue.title.startsWith(marker)
    );
    if (match) {
      return match;
    }
    if (issues.length < 100) {
      break;
    }
  }

  return null;
}

function buildIssueBody(metadata) {
  const lines = [
    "Automated ReverieVR diagnostic submission.",
    "",
    `- **Diagnostic ID:** ${metadata.diagnosticId}`,
    `- **SHA-256:** \`${metadata.sha256}\``,
    `- **Bundle bytes:** ${metadata.byteLength}`,
    `- **Submitted:** ${metadata.createdAt}`,
    `- **App/build:** ${publicText(metadata.appVersion, 120)} / ${publicText(metadata.buildType, 80)}`,
    `- **Device:** ${publicText(metadata.device, 180)}`,
    `- **Android:** ${publicText(metadata.android, 120)}`,
    `- **Logging mode:** ${publicText(metadata.loggingMode, 40)}`,
    `- **Private bundle reference:** \`${metadata.receiptReference}\``,
    "",
    "### What happened?",
    publicText(metadata.summary, 2000),
    "",
    "### Expected behavior",
    publicText(metadata.expected || "Not supplied.", 2000),
    "",
    "> Raw diagnostic ZIP is stored privately and is not attached to this public issue."
  ];
  return lines.join("\n");
}

function publicText(value, maxLength) {
  return String(value || "")
    .replace(/@/g, "＠")
    .replace(/[\u0000-\u001F\u007F]/g, " ")
    .trim()
    .slice(0, maxLength);
}

function githubHeaders(token) {
  return {
    "accept": "application/vnd.github+json",
    "authorization": `Bearer ${token}`,
    "x-github-api-version": "2026-03-10",
    "user-agent": USER_AGENT,
    "content-type": "application/json"
  };
}

async function githubJson(url, init) {
  const response = await fetch(url, init);
  const text = await response.text();
  let payload = {};
  if (text) {
    try {
      payload = JSON.parse(text);
    } catch (error) {
      payload = {message: text.slice(0, 500)};
    }
  }

  if (!response.ok) {
    throw new Error(`GitHub ${response.status}: ${payload.message || "request failed"}`);
  }
  return payload;
}

async function createGitHubAppJwt(appId, pem) {
  const now = Math.floor(Date.now() / 1000);
  const header = base64UrlJson({alg: "RS256", typ: "JWT"});
  const payload = base64UrlJson({iat: now - 60, exp: now + 540, iss: String(appId)});
  const signingInput = `${header}.${payload}`;
  const keyBytes = pemToPkcs8(pem);
  const key = await crypto.subtle.importKey(
    "pkcs8",
    keyBytes,
    {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"},
    false,
    ["sign"]
  );
  const signature = await crypto.subtle.sign(
    "RSASSA-PKCS1-v1_5",
    key,
    new TextEncoder().encode(signingInput)
  );
  return `${signingInput}.${base64UrlBytes(new Uint8Array(signature))}`;
}

function pemToPkcs8(pem) {
  const normalized = String(pem || "").trim();
  if (!normalized) {
    throw new Error("GitHub App private key is empty.");
  }

  if (normalized.includes("BEGIN PRIVATE KEY")) {
    return pemBody(normalized, "PRIVATE KEY");
  }
  if (!normalized.includes("BEGIN RSA PRIVATE KEY")) {
    throw new Error("Unsupported GitHub App private-key PEM format.");
  }

  const pkcs1 = pemBody(normalized, "RSA PRIVATE KEY");
  const rsaAlgorithmIdentifier = new Uint8Array([
    0x30, 0x0d,
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01,
    0x05, 0x00
  ]);
  const version = new Uint8Array([0x02, 0x01, 0x00]);
  const privateKeyOctet = derElement(0x04, pkcs1);
  return derElement(0x30, concatBytes(version, rsaAlgorithmIdentifier, privateKeyOctet));
}

function pemBody(pem, label) {
  const base64 = pem
    .replace(`-----BEGIN ${label}-----`, "")
    .replace(`-----END ${label}-----`, "")
    .replace(/\s+/g, "");
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index++) {
    bytes[index] = binary.charCodeAt(index);
  }
  return bytes;
}

function derElement(tag, body) {
  return concatBytes(new Uint8Array([tag]), derLength(body.length), body);
}

function derLength(length) {
  if (length < 0x80) {
    return new Uint8Array([length]);
  }
  const parts = [];
  let remaining = length;
  while (remaining > 0) {
    parts.unshift(remaining & 0xff);
    remaining >>>= 8;
  }
  return new Uint8Array([0x80 | parts.length, ...parts]);
}

function concatBytes(...arrays) {
  const size = arrays.reduce((sum, value) => sum + value.length, 0);
  const result = new Uint8Array(size);
  let offset = 0;
  for (const value of arrays) {
    result.set(value, offset);
    offset += value.length;
  }
  return result;
}

function base64UrlJson(value) {
  return base64UrlBytes(new TextEncoder().encode(JSON.stringify(value)));
}

function base64UrlBytes(bytes) {
  let binary = "";
  for (let index = 0; index < bytes.length; index++) {
    binary += String.fromCharCode(bytes[index]);
  }
  return btoa(binary)
    .replace(/=/g, "")
    .replace(/\+/g, "-")
    .replace(/\//g, "_");
}

async function sha256Hex(bytes) {
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return Array.from(new Uint8Array(digest))
    .map(value => value.toString(16).padStart(2, "0"))
    .join("");
}

async function readReceipt(bucket, key) {
  const object = await bucket.get(key);
  if (!object) {
    return null;
  }
  try {
    return JSON.parse(await object.text());
  } catch (error) {
    return null;
  }
}

async function expireOldBundles(env) {
  if (!env.DIAGNOSTIC_BUNDLES) {
    return;
  }

  const retentionDays = positiveInt(env.RETENTION_DAYS, 30);
  const cutoff = Date.now() - retentionDays * 24 * 60 * 60 * 1000;
  let cursor;

  do {
    const page = await env.DIAGNOSTIC_BUNDLES.list({
      prefix: BUNDLE_PREFIX,
      cursor,
      limit: 500
    });

    const expired = page.objects
      .filter(object => object.uploaded && object.uploaded.getTime() < cutoff)
      .map(object => object.key);

    if (expired.length > 0) {
      await env.DIAGNOSTIC_BUNDLES.delete(expired);
    }
    cursor = page.truncated ? page.cursor : undefined;
  } while (cursor);
}

function positiveInt(value, fallback) {
  const parsed = Number.parseInt(String(value || ""), 10);
  return Number.isFinite(parsed) && parsed > 0 ? parsed : fallback;
}

function json(value, status) {
  return new Response(JSON.stringify(value), {status, headers: JSON_HEADERS});
}
