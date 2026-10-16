package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class SteamVrLinkProbeTest {
    private static final String FINGERPRINT =
        "00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDDEEFF";
    private static final String TOKEN = "00112233445566778899AABBCCDDEEFF";

    @Test public void usbForcesLoopbackAndNeverInterpretsArbitraryHost() {
        SteamVrLinkProbe.Parameters params =
            new SteamVrLinkProbe.Parameters("evil.example", FINGERPRINT,
                TOKEN, true);
        assertEquals("127.0.0.1", params.host);
        assertEquals(32, params.fingerprint.length);
        assertEquals(TOKEN, params.token);
    }

    @Test public void privateWifiAndUsbTetherRangesAreAllowed() {
        for (String ip : new String[] {
            "192.168.1.20", "10.0.0.2", "172.16.1.4", "172.31.255.3"
        }) {
            assertEquals(ip, SteamVrLinkProbe.validateHost(ip, false));
        }
    }

    @Test public void publicDnsLoopbackAndMalformedIpsAreRejected() {
        for (String invalid : new String[] {
            "8.8.8.8", "127.0.0.1", "reverie.com", "172.32.0.3",
            "192.169.1.3", "192.168.1.256", "192.168.1.1:80",
            "192.168.1.0", "192.168.1.255"
        }) {
            try {
                SteamVrLinkProbe.validateHost(invalid, false);
                fail("Accepted invalid host: " + invalid);
            } catch (IllegalArgumentException expected) {
                // Refuse public, malformed or unexpected destination.
            }
        }
    }

    @Test public void pinsUseExact256BitFingerprintAnd128BitToken() {
        assertEquals(32, SteamVrLinkProbe.fromHex(FINGERPRINT, 32).length);
        assertEquals(16, SteamVrLinkProbe.fromHex(TOKEN, 16).length);
        for (String invalid : new String[] {"", "ABC", "Z" + TOKEN.substring(1),
            TOKEN + "00"}) {
            try {
                SteamVrLinkProbe.fromHex(invalid, 16);
                fail("Accepted invalid token");
            } catch (IllegalArgumentException expected) {
                // Deliberate offline/out-of-band pairing only.
            }
        }
    }
}
