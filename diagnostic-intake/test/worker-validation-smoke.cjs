const assert = require("assert");
const fs = require("fs");
const path = require("path");

const workerPath = path.join(
  __dirname,
  "..",
  "src",
  "index.js"
);
const source = fs.readFileSync(workerPath, "utf8");

const transformed = source
  .replace(
    "export default {",
    "const __worker_default__ = {"
  )
  .replace(
    "export class DiagnosticRateLimiter",
    "class DiagnosticRateLimiter"
  );

const api = new Function(
  transformed
    + "\nreturn {inspectZip, publicReceipt};"
)();

function asciiBytes(value) {
  return Uint8Array.from(
    Array.from(value).map(
      character =>
        character.charCodeAt(0) & 0xff
    )
  );
}

function put16(bytes, offset, value) {
  bytes[offset] = value & 0xff;
  bytes[offset + 1] =
    (value >>> 8) & 0xff;
}

function put32(bytes, offset, value) {
  put16(
    bytes,
    offset,
    value & 0xffff
  );
  put16(
    bytes,
    offset + 2,
    (value >>> 16) & 0xffff
  );
}

function fakeZip(
  localName,
  centralName = localName
) {
  const local =
    asciiBytes(localName);
  const central =
    asciiBytes(centralName);
  const localLength =
    30 + local.length;
  const centralLength =
    46 + central.length;
  const bytes =
    new Uint8Array(
      localLength
        + centralLength
        + 22
    );

  put32(bytes, 0, 0x04034b50);
  put16(bytes, 26, local.length);
  bytes.set(local, 30);

  const centralOffset =
    localLength;
  put32(
    bytes,
    centralOffset,
    0x02014b50
  );
  put16(
    bytes,
    centralOffset + 28,
    central.length
  );
  put32(
    bytes,
    centralOffset + 20,
    0
  );
  put32(
    bytes,
    centralOffset + 24,
    0
  );
  put32(
    bytes,
    centralOffset + 42,
    0
  );
  bytes.set(
    central,
    centralOffset + 46
  );

  const endOffset =
    centralOffset
      + centralLength;
  put32(
    bytes,
    endOffset,
    0x06054b50
  );
  put16(
    bytes,
    endOffset + 8,
    1
  );
  put16(
    bytes,
    endOffset + 10,
    1
  );
  put32(
    bytes,
    endOffset + 12,
    centralLength
  );
  put32(
    bytes,
    endOffset + 16,
    centralOffset
  );
  put16(
    bytes,
    endOffset + 20,
    0
  );

  return bytes;
}

const limits = {
  maxEntries: 128,
  maxExpandedBytes:
    128 * 1024 * 1024
};

assert.deepStrictEqual(
  api.inspectZip(
    fakeZip("manifest.txt"),
    limits
  ),
  {
    ok: true,
    entries: 1,
    expandedBytes: 0
  }
);

assert.strictEqual(
  api.inspectZip(
    fakeZip("payload.bin"),
    limits
  ).reason,
  "unexpected_zip_entry"
);

assert.strictEqual(
  api.inspectZip(
    fakeZip(
      "logs/reverie-test.log"
    ),
    limits
  ).reason,
  "missing_manifest"
);

assert.strictEqual(
  api.inspectZip(
    fakeZip(
      "manifest.txt",
      "logs/reverie-test.log"
    ),
    limits
  ).reason,
  "local_name_mismatch"
);

const publicView =
  api.publicReceipt({
    diagnosticId:
      "revdiag-12345678-1234-4123-8123-123456789abc",
    receiptReference:
      "revdiag-12345678-1234-4123-8123-123456789abc",
    sha256:
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    byteLength: 1234,
    issueNumber: 42,
    issueUrl:
      "https://github.com/mrcalzon02/ReverieVR/issues/42",
    objectKey:
      "diagnostics/private.zip",
    metadata: {
      private: "must-not-leak"
    }
  });

assert.strictEqual(
  publicView.issueNumber,
  42
);
assert.ok(
  !Object.prototype.hasOwnProperty.call(
    publicView,
    "objectKey"
  )
);
assert.ok(
  !Object.prototype.hasOwnProperty.call(
    publicView,
    "metadata"
  )
);

console.log(
  "diagnostic worker validation smoke tests passed"
);
