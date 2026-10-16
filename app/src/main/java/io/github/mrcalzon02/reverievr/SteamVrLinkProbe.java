package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.Locale;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

/** RV-0246 S1 diagnostic-only, pinned TLS over USB/ADB or private LAN. */
final class SteamVrLinkProbe {
    static final int PORT = 38726;
    private static final int CONNECT_TIMEOUT_MS = 5000;
    private static final int IO_TIMEOUT_MS = 7000;
    private static final String PROBE_PREFIX = "RVLINK/1 PROBE ";
    private static final String ACCEPT = "RVLINK/1 READY";

    private SteamVrLinkProbe() {}

    static final class Parameters {
        final String host;
        final byte[] fingerprint;
        final String token;

        Parameters(String host, String fingerprintHex, String tokenHex,
                   boolean usbReverse) {
            this.host = validateHost(host, usbReverse);
            this.fingerprint = fromHex(fingerprintHex, 32);
            this.token = toHex(fromHex(tokenHex, 16));
        }
    }

    static final class Result {
        final boolean paired;
        final String message;

        private Result(boolean paired, String message) {
            this.paired = paired;
            this.message = message;
        }
    }

    static Result connect(Parameters parameters) {
        try {
            if (Thread.currentThread().isInterrupted())
                return new Result(false, "Connection cancelled.");
            final byte[] expected = parameters.fingerprint.clone();
            final X509TrustManager pinnedTrust = new X509TrustManager() {
                @Override public void checkClientTrusted(
                    X509Certificate[] certificates, String authType
                ) throws CertificateException {
                    throw new CertificateException("Client TLS is unsupported.");
                }

                @Override public void checkServerTrusted(
                    X509Certificate[] certificates, String authType
                ) throws CertificateException {
                    if (certificates == null || certificates.length < 1)
                        throw new CertificateException("PC certificate missing.");
                    try {
                        byte[] actual = MessageDigest.getInstance("SHA-256")
                            .digest(certificates[0].getEncoded());
                        if (!MessageDigest.isEqual(actual, expected))
                            throw new CertificateException("PC fingerprint mismatch.");
                    } catch (java.security.GeneralSecurityException failure) {
                        throw new CertificateException("PC identity mismatch.", failure);
                    }
                }

                @Override public X509Certificate[] getAcceptedIssuers() {
                    return new X509Certificate[0];
                }
            };
            SSLContext context = SSLContext.getInstance("TLSv1.2");
            context.init(null, new TrustManager[] {pinnedTrust}, null);
            try (Socket raw = new Socket()) {
                raw.connect(new InetSocketAddress(parameters.host, PORT),
                    CONNECT_TIMEOUT_MS);
                raw.setSoTimeout(IO_TIMEOUT_MS);
                try (SSLSocket secured = (SSLSocket) context.getSocketFactory()
                    .createSocket(raw, parameters.host, PORT, false)) {
                    secured.setSoTimeout(IO_TIMEOUT_MS);
                    secured.setEnabledProtocols(new String[]{"TLSv1.2"});
                    secured.startHandshake();
                    if (Thread.currentThread().isInterrupted())
                        return new Result(false, "Connection cancelled.");
                    long sentAtNanos = System.nanoTime();
                    OutputStream out = secured.getOutputStream();
                    out.write((PROBE_PREFIX + parameters.token + "\n")
                        .getBytes(StandardCharsets.US_ASCII));
                    out.flush();
                    String response = readBoundedLine(secured.getInputStream(), 60);
                    if (!ACCEPT.equals(response))
                        return new Result(false, "PC rejected pairing token/protocol.");
                    double rttMillis = Math.max(0.0,
                        (System.nanoTime() - sentAtNanos) / 1_000_000.0);
                    return new Result(true, String.format(Locale.US,
                        "Authenticated TLS READY (request/response %.1f ms). "
                            + "This is NOT headset frame latency. "
                            + "SteamVR video/tracking are not connected yet.",
                        rttMillis));
                }
            }
        } catch (java.net.SocketTimeoutException timeout) {
            return new Result(false, "Connection timed out. Check cable, IP, firewall or host.");
        } catch (javax.net.ssl.SSLHandshakeException untrusted) {
            return new Result(false, "TLS identity verification failed. Recheck PC fingerprint.");
        } catch (IOException unreachable) {
            return new Result(false, "PC link unavailable. Check host address and port.");
        } catch (java.security.GeneralSecurityException tls) {
            return new Result(false, "Secure connection could not initialize.");
        }
    }

    private static String readBoundedLine(InputStream input, int cap)
        throws IOException {
        byte[] bytes = new byte[cap];
        int n = 0;
        while (n < cap) {
            int value = input.read();
            if (value < 0) throw new IOException("PC link closed early.");
            if (value == '\n') {
                return new String(bytes, 0, n, StandardCharsets.US_ASCII);
            }
            if (value < 32 || value > 126)
                throw new IOException("Invalid protocol response.");
            bytes[n++] = (byte)value;
        }
        throw new IOException("Oversized protocol response.");
    }

    static String validateHost(String host, boolean usbReverse) {
        if (usbReverse) return "127.0.0.1";
        if (host == null) throw new IllegalArgumentException("PC private IPv4 is required.");
        String candidate = host.trim();
        String[] parts = candidate.split("\\.", -1);
        if (parts.length != 4) throw new IllegalArgumentException("Use PC private IPv4.");
        int[] octets = new int[4];
        for (int i = 0; i < 4; i++) {
            if (!parts[i].matches("[0-9]{1,3}"))
                throw new IllegalArgumentException("Use PC private IPv4.");
            octets[i] = Integer.parseInt(parts[i]);
            if (octets[i] > 255)
                throw new IllegalArgumentException("Invalid IP address.");
        }
        boolean local = octets[0] == 10 ||
            (octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31) ||
            (octets[0] == 192 && octets[1] == 168);
        if (!local || octets[3] == 0 || octets[3] == 255)
            throw new IllegalArgumentException(
                "Only private LAN / USB-tether IPv4 addresses are supported.");
        return octets[0] + "." + octets[1] + "." + octets[2] + "." + octets[3];
    }

    static byte[] fromHex(String value, int length) {
        if (value == null) throw new IllegalArgumentException("Missing pairing value.");
        String normalized = value.trim().replace(":", "").replace("-", "")
            .replace(" ", "").toUpperCase(Locale.US);
        if (normalized.length() != length * 2
            || !normalized.matches("[0-9A-F]+"))
            throw new IllegalArgumentException(
                "Expected exactly " + (length * 2) + " hexadecimal characters.");
        byte[] output = new byte[length];
        for (int i = 0; i < length; i++)
            output[i] = (byte) Integer.parseInt(
                normalized.substring(i * 2, i * 2 + 2), 16);
        return output;
    }

    private static String toHex(byte[] bytes) {
        final char[] alphabet = "0123456789ABCDEF".toCharArray();
        char[] result = new char[bytes.length * 2];
        for (int i = 0; i < bytes.length; i++) {
            result[i * 2] = alphabet[(bytes[i] >>> 4) & 15];
            result[i * 2 + 1] = alphabet[bytes[i] & 15];
        }
        return new String(result);
    }
}
