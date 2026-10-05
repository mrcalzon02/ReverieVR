package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class DaydreamControllerPacketTest {
    @Test
    public void zeroPacketProducesNeutralState() {
        ControllerSnapshot snapshot =
            DaydreamControllerPacket.parse(new byte[DaydreamControllerPacket.PACKET_BYTES]);

        assertEquals(0, snapshot.timestampMillisModulo512);
        assertEquals(0, snapshot.packetIndex);
        assertEquals(0.0f, snapshot.orientationX, 0.0001f);
        assertEquals(0.0f, snapshot.orientationY, 0.0001f);
        assertEquals(0.0f, snapshot.orientationZ, 0.0001f);
        assertEquals(1.0f, snapshot.orientationW, 0.0001f);
        assertFalse(snapshot.touching);
        assertFalse(snapshot.volumeUpPressed);
        assertFalse(snapshot.volumeDownPressed);
        assertFalse(snapshot.menuPressed);
        assertFalse(snapshot.homePressed);
        assertFalse(snapshot.touchpadPressed);
    }

    @Test
    public void packetDecodesSignedFieldsTouchAndButtons() {
        BitWriter writer = new BitWriter(DaydreamControllerPacket.PACKET_BYTES);

        writer.writeUnsigned(257, 9);
        writer.writeUnsigned(17, 5);

        writer.writeSigned(1024, 13);
        writer.writeSigned(-512, 13);
        writer.writeSigned(256, 13);

        writer.writeSigned(2048, 13);
        writer.writeSigned(-2048, 13);
        writer.writeSigned(512, 13);

        writer.writeSigned(1000, 13);
        writer.writeSigned(-1000, 13);
        writer.writeSigned(250, 13);

        writer.writeUnsigned(123, 8);
        writer.writeUnsigned(45, 8);

        writer.writeBoolean(true);
        writer.writeBoolean(false);
        writer.writeBoolean(true);
        writer.writeBoolean(false);
        writer.writeBoolean(true);

        writer.writeUnsigned(0x35, 7);
        writer.writeBoolean(false);

        ControllerSnapshot snapshot = DaydreamControllerPacket.parse(writer.bytes());

        assertEquals(257, snapshot.timestampMillisModulo512);
        assertEquals(17, snapshot.packetIndex);
        assertTrue(snapshot.touching);
        assertEquals(123, snapshot.touchX);
        assertEquals(45, snapshot.touchY);
        assertTrue(snapshot.volumeUpPressed);
        assertFalse(snapshot.volumeDownPressed);
        assertTrue(snapshot.menuPressed);
        assertFalse(snapshot.homePressed);
        assertTrue(snapshot.touchpadPressed);
        assertEquals(0x35, snapshot.warningBits);

        assertEquals((2048.0f / 4095.0f) * 8.0f, snapshot.accelXG, 0.0001f);
        assertEquals((-2048.0f / 4095.0f) * 8.0f, snapshot.accelYG, 0.0001f);
        assertEquals((1000.0f / 4095.0f) * 2048.0f, snapshot.gyroXDps, 0.01f);
        assertEquals((-1000.0f / 4095.0f) * 2048.0f, snapshot.gyroYDps, 0.01f);
    }

    @Test(expected = IllegalArgumentException.class)
    public void shortPacketIsRejected() {
        DaydreamControllerPacket.parse(new byte[19]);
    }

    private static final class BitWriter {
        private final byte[] data;
        private int bitPosition;

        BitWriter(int bytes) {
            data = new byte[bytes];
        }

        void writeSigned(int value, int count) {
            int mask = (1 << count) - 1;
            writeUnsigned(value & mask, count);
        }

        void writeUnsigned(int value, int count) {
            for (int index = count - 1; index >= 0; index--) {
                int bit = (value >>> index) & 1;
                int byteIndex = bitPosition >>> 3;
                int offset = 7 - (bitPosition & 7);
                data[byteIndex] |= bit << offset;
                bitPosition++;
            }
        }

        void writeBoolean(boolean value) {
            writeUnsigned(value ? 1 : 0, 1);
        }

        byte[] bytes() {
            assertEquals(DaydreamControllerPacket.PACKET_BYTES * 8, bitPosition);
            return data;
        }
    }
}
