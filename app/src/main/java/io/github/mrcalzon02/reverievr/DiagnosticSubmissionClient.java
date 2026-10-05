package io.github.mrcalzon02.reverievr;

import android.os.Build;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.UUID;

final class DiagnosticSubmissionClient {
    private static final int CONNECT_TIMEOUT_MILLIS = 15000;
    private static final int READ_TIMEOUT_MILLIS = 60000;
    private static final int BUFFER_SIZE = 64 * 1024;
    private static final long MAX_UPLOAD_BYTES = 25L * 1024L * 1024L;

    private final String endpoint;

    DiagnosticSubmissionClient() {
        endpoint =
            BuildConfig.DIAGNOSTIC_INTAKE_URL == null
                ? ""
                : BuildConfig.DIAGNOSTIC_INTAKE_URL.trim();
    }

    boolean isConfigured() {
        if (endpoint.isEmpty()) {
            return false;
        }

        try {
            URI uri = URI.create(endpoint);
            return "https".equalsIgnoreCase(uri.getScheme())
                && uri.getHost() != null
                && !uri.getHost().trim().isEmpty();
        } catch (IllegalArgumentException exception) {
            return false;
        }
    }

    Result submit(
        File bundle,
        String diagnosticId,
        String summary,
        String expected
    ) throws IOException {
        if (!isConfigured()) {
            throw new IOException(
                "Secure diagnostic intake is not configured in this build."
            );
        }
        if (bundle == null || !bundle.isFile()) {
            throw new IOException("Diagnostic bundle is missing.");
        }
        if (bundle.length() <= 0 || bundle.length() > MAX_UPLOAD_BYTES) {
            throw new IOException(
                "Diagnostic bundle is outside the allowed upload size."
            );
        }
        if (diagnosticId == null
            || !diagnosticId.matches(
                "^revdiag-[0-9a-fA-F-]{36}$"
            )) {
            throw new IOException("Diagnostic ID is invalid.");
        }

        String sha256 = sha256(bundle);
        String boundary =
            "----ReverieVR-"
                + UUID.randomUUID().toString().replace("-", "");

        HttpURLConnection connection =
            (HttpURLConnection) new URL(endpoint).openConnection();
        connection.setRequestMethod("POST");
        connection.setConnectTimeout(CONNECT_TIMEOUT_MILLIS);
        connection.setReadTimeout(READ_TIMEOUT_MILLIS);
        connection.setDoOutput(true);
        connection.setUseCaches(false);
        connection.setChunkedStreamingMode(BUFFER_SIZE);
        connection.setRequestProperty(
            "Content-Type",
            "multipart/form-data; boundary=" + boundary
        );
        connection.setRequestProperty(
            "Accept",
            "application/json"
        );
        connection.setRequestProperty(
            "User-Agent",
            "ReverieVR/" + BuildConfig.VERSION_NAME
        );

        try {
            try (DataOutputStream output =
                     new DataOutputStream(
                         new BufferedOutputStream(
                             connection.getOutputStream()
                         )
                     )) {
                writeField(
                    output,
                    boundary,
                    "diagnostic_id",
                    diagnosticId
                );
                writeField(
                    output,
                    boundary,
                    "privacy_reviewed",
                    "true"
                );
                writeField(
                    output,
                    boundary,
                    "bundle_sha256",
                    sha256
                );
                writeField(
                    output,
                    boundary,
                    "summary",
                    safeText(summary, 2000)
                );
                writeField(
                    output,
                    boundary,
                    "expected",
                    safeText(expected, 2000)
                );
                writeField(
                    output,
                    boundary,
                    "app_version",
                    BuildConfig.VERSION_NAME
                );
                writeField(
                    output,
                    boundary,
                    "build_type",
                    BuildConfig.BUILD_TYPE
                );
                writeField(
                    output,
                    boundary,
                    "device",
                    Build.MANUFACTURER
                        + " "
                        + Build.MODEL
                        + " ("
                        + Build.DEVICE
                        + ")"
                );
                writeField(
                    output,
                    boundary,
                    "android",
                    "Android "
                        + Build.VERSION.RELEASE
                        + " / API "
                        + Build.VERSION.SDK_INT
                );
                writeField(
                    output,
                    boundary,
                    "logging_mode",
                    ReverieLog.getMode().name()
                );
                writeFile(
                    output,
                    boundary,
                    "bundle",
                    diagnosticId + ".zip",
                    bundle
                );

                output.writeBytes("--" + boundary + "--\r\n");
                output.flush();
            }

            int status = connection.getResponseCode();
            String body = readResponse(
                status >= 200 && status < 300
                    ? connection.getInputStream()
                    : connection.getErrorStream()
            );

            JSONObject json;
            try {
                json = body.isEmpty()
                    ? new JSONObject()
                    : new JSONObject(body);
            } catch (JSONException exception) {
                throw new IOException(
                    "Diagnostic intake returned an invalid response.",
                    exception
                );
            }

            if (status < 200 || status >= 300) {
                String error = json.optString("error", "upload_failed");
                String receipt =
                    json.optString("receiptReference", "");
                throw new SubmissionException(
                    "Diagnostic submission failed ("
                        + status
                        + "): "
                        + error,
                    receipt
                );
            }

            String serverHash =
                json.optString("sha256", "").trim().toLowerCase();
            if (!serverHash.isEmpty()
                && !serverHash.equals(sha256)) {
                throw new IOException(
                    "Diagnostic intake hash verification failed."
                );
            }

            String issueUrl =
                json.optString("issueUrl", "").trim();
            if (!issueUrl.startsWith(
                    "https://github.com/mrcalzon02/ReverieVR/issues/"
                )) {
                throw new IOException(
                    "Diagnostic intake did not return a valid issue URL."
                );
            }

            return new Result(
                diagnosticId,
                sha256,
                json.optString(
                    "receiptReference",
                    diagnosticId
                ),
                issueUrl,
                json.optInt("issueNumber", -1)
            );
        } finally {
            connection.disconnect();
        }
    }

    static String newDiagnosticId() {
        return "revdiag-" + UUID.randomUUID();
    }

    static String sha256(File file) throws IOException {
        MessageDigest digest;
        try {
            digest = MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException exception) {
            throw new IOException(
                "SHA-256 is unavailable.",
                exception
            );
        }

        byte[] buffer = new byte[BUFFER_SIZE];
        try (BufferedInputStream input =
                 new BufferedInputStream(
                     new FileInputStream(file)
                 )) {
            int read;
            while ((read = input.read(buffer)) >= 0) {
                if (read > 0) {
                    digest.update(buffer, 0, read);
                }
            }
        }

        StringBuilder hex = new StringBuilder(64);
        for (byte value : digest.digest()) {
            hex.append(
                String.format(
                    java.util.Locale.US,
                    "%02x",
                    value & 0xff
                )
            );
        }
        return hex.toString();
    }

    private static void writeField(
        DataOutputStream output,
        String boundary,
        String name,
        String value
    ) throws IOException {
        output.writeBytes("--" + boundary + "\r\n");
        output.writeBytes(
            "Content-Disposition: form-data; name=\""
                + name
                + "\"\r\n"
        );
        output.writeBytes(
            "Content-Type: text/plain; charset=UTF-8\r\n\r\n"
        );
        output.write(
            safeText(value, 4000)
                .getBytes(StandardCharsets.UTF_8)
        );
        output.writeBytes("\r\n");
    }

    private static void writeFile(
        DataOutputStream output,
        String boundary,
        String fieldName,
        String filename,
        File file
    ) throws IOException {
        output.writeBytes("--" + boundary + "\r\n");
        output.writeBytes(
            "Content-Disposition: form-data; name=\""
                + fieldName
                + "\"; filename=\""
                + filename
                + "\"\r\n"
        );
        output.writeBytes(
            "Content-Type: application/zip\r\n\r\n"
        );

        byte[] buffer = new byte[BUFFER_SIZE];
        try (BufferedInputStream input =
                 new BufferedInputStream(
                     new FileInputStream(file)
                 )) {
            int read;
            while ((read = input.read(buffer)) >= 0) {
                if (read > 0) {
                    output.write(buffer, 0, read);
                }
            }
        }
        output.writeBytes("\r\n");
    }

    private static String readResponse(
        java.io.InputStream input
    ) throws IOException {
        if (input == null) {
            return "";
        }

        try (java.io.InputStream source = input;
             ByteArrayOutputStream output =
                 new ByteArrayOutputStream()) {
            byte[] buffer = new byte[8192];
            int read;
            while ((read = source.read(buffer)) >= 0) {
                if (read > 0) {
                    output.write(buffer, 0, read);
                }
                if (output.size() > 256 * 1024) {
                    throw new IOException(
                        "Diagnostic intake response is too large."
                    );
                }
            }
            return output.toString(
                StandardCharsets.UTF_8.name()
            );
        }
    }

    private static String safeText(
        String value,
        int maxLength
    ) {
        String safe = value == null ? "" : value;
        safe = safe
            .replace('\u0000', ' ')
            .replace('\r', ' ')
            .trim();
        return safe.length() <= maxLength
            ? safe
            : safe.substring(0, maxLength);
    }

    static final class Result {
        final String diagnosticId;
        final String sha256;
        final String receiptReference;
        final String issueUrl;
        final int issueNumber;

        Result(
            String diagnosticId,
            String sha256,
            String receiptReference,
            String issueUrl,
            int issueNumber
        ) {
            this.diagnosticId = diagnosticId;
            this.sha256 = sha256;
            this.receiptReference = receiptReference;
            this.issueUrl = issueUrl;
            this.issueNumber = issueNumber;
        }
    }

    static final class SubmissionException extends IOException {
        final String receiptReference;

        SubmissionException(
            String message,
            String receiptReference
        ) {
            super(message);
            this.receiptReference =
                receiptReference == null
                    ? ""
                    : receiptReference;
        }
    }
}
