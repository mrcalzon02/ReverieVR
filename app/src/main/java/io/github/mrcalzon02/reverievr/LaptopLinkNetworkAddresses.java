package io.github.mrcalzon02.reverievr;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;

/** Uses the phone's existing interfaces; never scans Wi-Fi or other hosts. */
final class LaptopLinkNetworkAddresses {
    private LaptopLinkNetworkAddresses() {}

    static List<String> trustedLanIpv4() {
        List<String> addresses = new ArrayList<>();
        try {
            Enumeration<NetworkInterface> devices =
                NetworkInterface.getNetworkInterfaces();
            if (devices == null) return addresses;
            while (devices.hasMoreElements()) {
                NetworkInterface device = devices.nextElement();
                if (!device.isUp() || device.isLoopback()
                    || !eligibleInterface(device.getName())) continue;
                Enumeration<InetAddress> ips = device.getInetAddresses();
                while (ips.hasMoreElements()) {
                    InetAddress ip = ips.nextElement();
                    if (ip instanceof Inet4Address && ip.isSiteLocalAddress()
                        && !ip.isLoopbackAddress())
                        addresses.add(ip.getHostAddress());
                }
            }
        } catch (SocketException unavailable) {
            // USB loopback/ADB forwarding remains an independent option.
        }
        Collections.sort(addresses);
        return addresses;
    }

    static boolean eligibleInterface(String name) {
        if (name == null) return false;
        return name.startsWith("wlan") || name.startsWith("ap")
            || name.startsWith("swlan") || name.startsWith("eth")
            || name.startsWith("rndis") || name.startsWith("usb");
    }
}
