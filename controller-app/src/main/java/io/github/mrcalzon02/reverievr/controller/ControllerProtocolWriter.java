package io.github.mrcalzon02.reverievr.controller;

import java.io.ByteArrayOutputStream;

final class ControllerProtocolWriter {
    static final int TYPE_MOTION = 1;
    static final int TYPE_GYROSCOPE = 2;
    static final int TYPE_ACCELEROMETER = 3;
    static final int TYPE_ORIENTATION = 5;
    static final int TYPE_KEY = 6;
    static final int TYPE_REVERIE_STATUS = 100;

    static final int KEY_HOME = 0x03;
    static final int KEY_VOLUME_UP = 0x18;
    static final int KEY_VOLUME_DOWN = 0x19;
    static final int KEY_CLICK = 0x42;
    static final int KEY_APP = 0x52;

    static final int ACTION_DOWN = 0;
    static final int ACTION_UP = 1;
    static final int ACTION_MOVE = 2;
    static final int ACTION_CANCEL = 3;

    private ControllerProtocolWriter() {
    }

    static byte[] motion(
        long timestamp,
        int action,
        float normalizedX,
        float normalizedY
    ) {
        byte[] pointer = message(
            varintField(1, 0),
            floatField(2, clamp01(normalizedX)),
            floatField(3, clamp01(normalizedY))
        );

        byte[] body = message(
            varintField(1, timestamp),
            varintField(2, action),
            bytesField(3, pointer)
        );

        return envelope(TYPE_MOTION, 2, body);
    }

    static byte[] gyroscope(
        long timestamp,
        float x,
        float y,
        float z
    ) {
        return envelope(
            TYPE_GYROSCOPE,
            3,
            vectorBody(timestamp, x, y, z)
        );
    }

    static byte[] accelerometer(
        long timestamp,
        float x,
        float y,
        float z
    ) {
        return envelope(
            TYPE_ACCELEROMETER,
            4,
            vectorBody(timestamp, x, y, z)
        );
    }

    static byte[] orientation(
        long timestamp,
        float x,
        float y,
        float z,
        float w
    ) {
        byte[] body = message(
            varintField(1, timestamp),
            floatField(2, x),
            floatField(3, y),
            floatField(4, z),
            floatField(5, w)
        );
        return envelope(TYPE_ORIENTATION, 6, body);
    }

    static byte[] key(int action, int keyCode) {
        byte[] body = message(
            varintField(1, action),
            varintField(2, keyCode)
        );
        return envelope(TYPE_KEY, 7, body);
    }

    static byte[] batteryStatus(int percentage) {
        int safe = Math.max(0, Math.min(100, percentage));
        byte[] body = message(varintField(1, safe));
        return envelope(TYPE_REVERIE_STATUS, 8, body);
    }

    private static byte[] vectorBody(
        long timestamp,
        float x,
        float y,
        float z
    ) {
        return message(
            varintField(1, timestamp),
            floatField(2, x),
            floatField(3, y),
            floatField(4, z)
        );
    }

    private static byte[] envelope(
        int type,
        int eventField,
        byte[] body
    ) {
        return message(
            varintField(1, type),
            bytesField(eventField, body)
        );
    }

    private static byte[] varintField(int field, long value) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        writeVarint(out, field << 3);
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

    private static float clamp01(float value) {
        return Math.max(0.0f, Math.min(1.0f, value));
    }
}
