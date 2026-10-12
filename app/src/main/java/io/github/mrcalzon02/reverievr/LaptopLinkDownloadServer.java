package io.github.mrcalzon02.reverievr;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Explicit, short-lived, one-download HTTP handoff of APK-bundled host tools.
 * No general-purpose file server, PC execution, remote commands or uploads.
 * Expose only on a user-trusted private LAN; HTTP is NOT transport-encrypted.
 */
final class LaptopLinkDownloadServer implements AutoCloseable {
    static final long MAX_SESSION_NANOS = 5L * 60L * 1_000_000_000L;
    private static final int MAX_REQUEST_BYTES = 8192;
    private static final int MAX_BAD_REQUESTS = 24;
    private static final int MAX_ZIP_BYTES = 512 * 1024;
    private final byte[] payload;
    private final String sha256;
    private final String secret;
    private final ServerSocket listener;
    private final AtomicBoolean closed = new AtomicBoolean();
    private final Thread worker;
    private volatile boolean delivered;

    interface Completion {
        void finished(boolean delivered);
    }

    LaptopLinkDownloadServer(LaptopLinkBundle.Assets assets,
                             InetAddress bindAddress,
                             Completion completion) throws IOException {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        LaptopLinkBundle.write(assets, output);
        payload = output.toByteArray();
        if (payload.length == 0 || payload.length > MAX_ZIP_BYTES)
            throw new IOException("Laptop Link package exceeds download limit.");
        sha256 = hexSha256(payload);
        byte[] random = new byte[20];
        new SecureRandom().nextBytes(random);
        secret = toHex(random);
        listener = new ServerSocket(0, 8, bindAddress);
        listener.setSoTimeout(1000);
        worker = new Thread(() -> serve(completion), "reverie-link-one-shot");
        worker.setDaemon(true);
        worker.start();
    }

    String url(String phoneIpv4) {
        if (phoneIpv4 == null || !phoneIpv4.matches(
                "(?:[0-9]{1,3}\\.){3}[0-9]{1,3}"))
            throw new IllegalArgumentException("Expected a literal phone IPv4 address.");
        return "http://" + phoneIpv4 + ":" + listener.getLocalPort()
            + "/reverie-link/" + secret;
    }

    String sha256() { return sha256; }
    int port() { return listener.getLocalPort(); }
    boolean wasDelivered() { return delivered; }
    boolean isRunning() { return !closed.get(); }

    private void serve(Completion completion) {
        final long deadline = System.nanoTime() + MAX_SESSION_NANOS;
        int rejected = 0;
        try {
            while (!closed.get() && System.nanoTime() < deadline
                    && rejected < MAX_BAD_REQUESTS) {
                try (Socket client = listener.accept()) {
                    client.setSoTimeout(7000);
                    if (handle(client)) {
                        delivered = true;
                        break;
                    }
                    rejected++;
                } catch (SocketTimeoutException timeout) {
                    // Periodically check expiration and explicit stop.
                } catch (IOException error) {
                    if (closed.get()) break;
                    rejected++;
                }
            }
        } catch (IOException failure) {
            // Listener was closed or network became unavailable.
        } finally {
            close();
            if (completion != null) completion.finished(delivered);
        }
    }

    private boolean handle(Socket client) throws IOException {
        InputStream input = client.getInputStream();
        OutputStream output = client.getOutputStream();
        ByteArrayOutputStream request = new ByteArrayOutputStream();
        int n;
        while ((n = input.read()) != -1 && n != '\n') {
            if (request.size() >= MAX_REQUEST_BYTES)
                return false;
            request.write(n);
        }
        if (n == -1) return false;
        String first = request.toString(StandardCharsets.US_ASCII.name()).trim();
        String expected = "GET /reverie-link/" + secret + " HTTP/1.1";
        String legacy = "GET /reverie-link/" + secret + " HTTP/1.0";
        if (!first.equals(expected) && !first.equals(legacy)) {
            byte[] body = "Not found".getBytes(StandardCharsets.US_ASCII);
            sendHeaders(output, "404 Not Found", "text/plain", body.length);
            output.write(body);
            output.flush();
            return false;
        }
        // No request body is accepted. No log of one-time URL/token.
        sendHeaders(output, "200 OK", "application/zip", payload.length);
        output.write(payload);
        output.flush();
        return true;
    }

    private static void sendHeaders(OutputStream output, String status,
                                    String type, int length) throws IOException {
        String text = "HTTP/1.1 " + status + "\r\n"
            + "Content-Type: " + type + "\r\n"
            + "Content-Length: " + length + "\r\n"
            + "Content-Disposition: attachment; filename=\\\""
            + LaptopLinkBundle.EXPORT_NAME + "\\\"\r\n"
            + "Cache-Control: no-store\r\n"
            + "X-Content-Type-Options: nosniff\r\n"
            + "Referrer-Policy: no-referrer\r\n"
            + "Connection: close\r\n\r\n";
        output.write(text.getBytes(StandardCharsets.US_ASCII));
    }

    private static String hexSha256(byte[] value) throws IOException {
        try {
            return toHex(MessageDigest.getInstance("SHA-256").digest(value));
        } catch (NoSuchAlgorithmException impossible) {
            throw new IOException("SHA-256 unavailable.", impossible);
        }
    }

    private static String toHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder(bytes.length * 2);
        for (byte b : bytes)
            sb.append(String.format(Locale.US, "%02x", b & 0xff));
        return sb.toString();
    }

    @Override public void close() {
        if (closed.compareAndSet(false,true)) {
            try { listener.close(); }
            catch (IOException ignored) { /* No socket remains to serve. */ }
        }
    }
}
