package io.github.mrcalzon02.reverievr.controller;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import org.junit.Test;

public final class ControllerProtocolWriterTest {
    @Test
    public void writesClickKeyEnvelope() {
        byte[] bytes = ControllerProtocolWriter.key(
            ControllerProtocolWriter.ACTION_DOWN,
            ControllerProtocolWriter.KEY_CLICK
        );

        assertArrayEquals(
            new byte[] {
                0x08, 0x06,
                0x3a, 0x04,
                0x08, 0x00,
                0x10, 0x42
            },
            bytes
        );
    }

    @Test
    public void writesBatteryExtensionEnvelope() {
        byte[] bytes =
            ControllerProtocolWriter.batteryStatus(73);

        assertArrayEquals(
            new byte[] {
                0x08, 0x64,
                0x42, 0x02,
                0x08, 0x49
            },
            bytes
        );
    }

    @Test
    public void clampsBatteryExtension() {
        byte[] bytes =
            ControllerProtocolWriter.batteryStatus(140);

        assertEquals(0x64, bytes[1] & 0xff);
        assertEquals(0x64, bytes[5] & 0xff);
    }
}
