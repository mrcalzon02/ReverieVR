package io.github.mrcalzon02.reverievr;

final class DaydreamControllerPacket {
    static final int PACKET_BYTES = 20;

    private DaydreamControllerPacket() {
    }

    static ControllerSnapshot parse(byte[] packet) {
        if (packet == null || packet.length < PACKET_BYTES) {
            throw new IllegalArgumentException("Daydream pose packet must contain 20 bytes.");
        }

        BitReader bits = new BitReader(packet);

        int millis = bits.readUnsigned(9);
        int packetIndex = bits.readUnsigned(5);

        int poseXRaw = bits.readSigned(13);
        int poseYRaw = bits.readSigned(13);
        int poseZRaw = bits.readSigned(13);

        int accelXRaw = bits.readSigned(13);
        int accelYRaw = bits.readSigned(13);
        int accelZRaw = bits.readSigned(13);

        int gyroXRaw = bits.readSigned(13);
        int gyroYRaw = bits.readSigned(13);
        int gyroZRaw = bits.readSigned(13);

        int touchX = bits.readUnsigned(8);
        int touchY = bits.readUnsigned(8);

        boolean volumeUp = bits.readBoolean();
        boolean volumeDown = bits.readBoolean();
        boolean menu = bits.readBoolean();
        boolean home = bits.readBoolean();
        boolean touchpadClick = bits.readBoolean();

        int warnings = bits.readUnsigned(7);
        bits.readBoolean(); // Final observed bit is not required by ReverieVR.

        float poseX = unpackOrientation(poseXRaw);
        float poseY = unpackOrientation(poseYRaw);
        float poseZ = unpackOrientation(poseZRaw);
        float[] quaternion = scaledAxisToQuaternion(poseX, poseY, poseZ);

        boolean touching = touchX != 0 || touchY != 0;

        return new ControllerSnapshot(
            System.nanoTime(),
            millis,
            packetIndex,
            quaternion[0],
            quaternion[1],
            quaternion[2],
            quaternion[3],
            unpackAccelerationG(accelXRaw),
            unpackAccelerationG(accelYRaw),
            unpackAccelerationG(accelZRaw),
            unpackGyroDps(gyroXRaw),
            unpackGyroDps(gyroYRaw),
            unpackGyroDps(gyroZRaw),
            touching,
            touchX,
            touchY,
            volumeUp,
            volumeDown,
            menu,
            home,
            touchpadClick,
            warnings
        );
    }

    private static float unpackOrientation(int value) {
        return (float) ((value / 4095.0) * Math.PI * 2.0);
    }

    private static float unpackAccelerationG(int value) {
        return (value / 4095.0f) * 8.0f;
    }

    private static float unpackGyroDps(int value) {
        return (value / 4095.0f) * 2048.0f;
    }

    private static float[] scaledAxisToQuaternion(float x, float y, float z) {
        double angle = Math.sqrt((double) x * x + (double) y * y + (double) z * z);

        if (angle < 1.0e-7) {
            return new float[] {0.0f, 0.0f, 0.0f, 1.0f};
        }

        double half = angle * 0.5;
        double scale = Math.sin(half) / angle;

        return new float[] {
            (float) (x * scale),
            (float) (y * scale),
            (float) (z * scale),
            (float) Math.cos(half)
        };
    }

    private static final class BitReader {
        private final byte[] data;
        private int bitPosition;

        BitReader(byte[] data) {
            this.data = data;
        }

        int readUnsigned(int count) {
            if (count < 1 || count > 31) {
                throw new IllegalArgumentException("Unsupported bit count: " + count);
            }

            int result = 0;
            for (int index = 0; index < count; index++) {
                int byteIndex = bitPosition >>> 3;
                int offset = 7 - (bitPosition & 7);
                if (byteIndex >= data.length) {
                    throw new IllegalArgumentException("Pose packet ended unexpectedly.");
                }

                result = (result << 1) | ((data[byteIndex] >>> offset) & 1);
                bitPosition++;
            }
            return result;
        }

        int readSigned(int count) {
            int raw = readUnsigned(count);
            int sign = 1 << (count - 1);
            if ((raw & sign) != 0) {
                raw -= 1 << count;
            }
            return raw;
        }

        boolean readBoolean() {
            return readUnsigned(1) != 0;
        }
    }
}
