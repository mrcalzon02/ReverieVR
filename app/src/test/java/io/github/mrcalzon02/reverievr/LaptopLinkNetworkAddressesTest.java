package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class LaptopLinkNetworkAddressesTest {
    @Test public void onlyExplicitPhysicalLocalInterfacesAreCandidates() {
        assertTrue(LaptopLinkNetworkAddresses.eligibleInterface("wlan0"));
        assertTrue(LaptopLinkNetworkAddresses.eligibleInterface("ap0"));
        assertTrue(LaptopLinkNetworkAddresses.eligibleInterface("rndis0"));
        assertTrue(LaptopLinkNetworkAddresses.eligibleInterface("usb0"));
        assertFalse(LaptopLinkNetworkAddresses.eligibleInterface("tun0"));
        assertFalse(LaptopLinkNetworkAddresses.eligibleInterface("rmnet_data0"));
        assertFalse(LaptopLinkNetworkAddresses.eligibleInterface("lo"));
        assertFalse(LaptopLinkNetworkAddresses.eligibleInterface(null));
    }
}
