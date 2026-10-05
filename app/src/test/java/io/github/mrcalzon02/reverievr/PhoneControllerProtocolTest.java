package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.io.ByteArrayOutputStream;

public final class PhoneControllerProtocolTest {
    @Test
    public void parsesOrientationPacket() {
        byte[] orientation = message(
            varintField(1, 123456L),
            floatField(2, 0.10f),
            floatField(3, -0.20f),
            floatField(4, 0.30f),
            floatField(5, 0.90f)
        );

        byte[] packet = message(
            varintField(1, PhoneControllerProtocol.TYPE_ORIENTATION),
            bytesField(6, orientation)
        );

        PhoneControllerProtocol.Event event =
            PhoneControllerProtocol.parse(packet);

        assertEquals(PhoneControllerProtocol.TYPE_ORIENTATION, event.type);
        assertEquals(123456L, event.timestamp);
        assertEquals(0.10f, event.x, 0.0001f);
        assertEquals(-0.20f, event.y, 0.0001f);
        assertEquals(0.30f, event.z, 0.0001f);
        assertEquals(0.90f, event.w, 0.0001f);
    }

    @Test
    public void parsesTouchMotionPacket() {
        byte[] pointer = message(
            varintField(1, 0),
            floatField(2, 0.25f),
            floatField(3, 0.75f)
        );

        byte[] motion = message(
            varintField(1, 9876L),
            varintField(2, PhoneControllerProtocol.ACTION_MOVE),
            bytesField(3, pointer)
        );

        byte[] packet = message(
            varintField(1, PhoneControllerProtocol.TYPE_MOTION),
            bytesField(2, motion)
        );

        PhoneControllerProtocol.Event event =
            PhoneControllerProtocol.parse(packet);

        assertEquals(PhoneControllerProtocol.TYPE_MOTION, event.type);
        assertEquals(PhoneControllerProtocol.ACTION_MOVE, event.action);
        assertTrue(event.hasPointer);
        assertEquals(0.25f, event.x, 0.0001f);
        assertEquals(0.75f, event.y, 0.0001f);
    }

    @Test
    public void parsesClickKeyPacket() {
        byte[] key = message(
            varintField(1, PhoneControllerProtocol.ACTION_DOWN),
            varintField(2, PhoneControllerProtocol.KEY_CLICK)
        );

        byte[] packet = message(
            varintField(1, PhoneControllerProtocol.TYPE_KEY),
            bytesField(7, key)
        );

        PhoneControllerProtocol.Event event =
            PhoneControllerProtocol.parse(packet);

        assertEquals(PhoneControllerProtocol.TYPE_KEY, event.type);
        assertEquals(PhoneControllerProtocol.ACTION_DOWN, event.action);
        assertEquals(PhoneControllerProtocol.KEY_CLICK, event.keyCode);
    }

    @Test(expected = IllegalArgumentException.class)
    public void rejectsTruncatedLengthDelimitedField() {
        PhoneControllerProtocol.parse(
            new byte[] {
                0x08, 0x05,
                0x32, 0x04,
                0x01
            }
        );
    }

    private static byte[] varintField(int field, long value) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        writeVarint(out, (field << 3));
        writeVarint(out, value);
        return out.toByteArray();
    }

    private static byte[] floatField(int field, float value) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        writeVarint(out, (field << 3) | 5);

        int bits = Float.floatToIntBits(value);
        out.write(bits & 0xff);
        out.write((bits >>> 8) & 0xff);
        out.write((bits >>> 16) & 0xff);
        out.write((bits >>> 24) & 0xff);
        return out.toByteArray();
    }

    private static byte[] bytesField(int field, byte[] value) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        writeVarint(out, (field << 3) | 2);
        writeVarint(out, value.length);
        out.write(value, 0, value.length);
        return out.toByteArray();
    }

    private static byte[] message(byte[]... fields) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        for (byte[] field : fields) {
            out.write(field, 0, field.length);
        }
        return out.toByteArray();
    }

    private static void writeVarint(
        ByteArrayOutputStream out,
        long value
    ) {
        long remaining = value;
        while ((remaining & ~0x7fL) != 0L) {
            out.write((int) ((remaining & 0x7fL) | 0x80L));
            remaining >>>= 7;
        }
        out.write((int) remaining);
    }
}
