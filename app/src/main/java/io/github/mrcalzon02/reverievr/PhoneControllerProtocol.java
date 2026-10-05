package io.github.mrcalzon02.reverievr;

import java.util.Arrays;

final class PhoneControllerProtocol {
    static final int TYPE_MOTION = 1;
    static final int TYPE_GYROSCOPE = 2;
    static final int TYPE_ACCELEROMETER = 3;
    static final int TYPE_ORIENTATION = 5;
    static final int TYPE_KEY = 6;

    static final int KEY_NONE = 0x00;
    static final int KEY_HOME = 0x03;
    static final int KEY_VOLUME_UP = 0x18;
    static final int KEY_VOLUME_DOWN = 0x19;
    static final int KEY_CLICK = 0x42;
    static final int KEY_APP = 0x52;

    static final int ACTION_DOWN = 0;
    static final int ACTION_UP = 1;
    static final int ACTION_MOVE = 2;
    static final int ACTION_CANCEL = 3;

    private PhoneControllerProtocol() {
    }

    static Event parse(byte[] message) {
        if (message == null || message.length == 0) {
            throw new IllegalArgumentException("Controller emulator message is empty.");
        }

        ProtoReader reader = new ProtoReader(message);
        int type = 0;
        byte[] motion = null;
        byte[] gyro = null;
        byte[] accel = null;
        byte[] orientation = null;
        byte[] key = null;

        while (reader.hasRemaining()) {
            int tag = reader.readVarint32();
            int field = tag >>> 3;
            int wire = tag & 7;

            if (field == 1 && wire == 0) {
                type = reader.readVarint32();
            } else if (field >= 2 && field <= 7 && wire == 2) {
                byte[] nested = reader.readBytes();
                switch (field) {
                    case 2:
                        motion = nested;
                        break;
                    case 3:
                        gyro = nested;
                        break;
                    case 4:
                        accel = nested;
                        break;
                    case 6:
                        orientation = nested;
                        break;
                    case 7:
                        key = nested;
                        break;
                    default:
                        break;
                }
            } else {
                reader.skip(wire);
            }
        }

        switch (type) {
            case TYPE_MOTION:
                return parseMotion(motion);
            case TYPE_GYROSCOPE:
                return parseVector(TYPE_GYROSCOPE, gyro, false);
            case TYPE_ACCELEROMETER:
                return parseVector(TYPE_ACCELEROMETER, accel, false);
            case TYPE_ORIENTATION:
                return parseVector(TYPE_ORIENTATION, orientation, true);
            case TYPE_KEY:
                return parseKey(key);
            default:
                return Event.unknown(type);
        }
    }

    private static Event parseMotion(byte[] bytes) {
        if (bytes == null) {
            return Event.unknown(TYPE_MOTION);
        }

        ProtoReader reader = new ProtoReader(bytes);
        long timestamp = 0L;
        int action = ACTION_CANCEL;
        float x = 0.0f;
        float y = 0.0f;
        boolean havePointer = false;

        while (reader.hasRemaining()) {
            int tag = reader.readVarint32();
            int field = tag >>> 3;
            int wire = tag & 7;

            if (field == 1 && wire == 0) {
                timestamp = reader.readVarint64();
            } else if (field == 2 && wire == 0) {
                action = reader.readVarint32();
            } else if (field == 3 && wire == 2) {
                float[] pointer = parsePointer(reader.readBytes());
                x = pointer[0];
                y = pointer[1];
                havePointer = true;
            } else {
                reader.skip(wire);
            }
        }

        Event event = new Event(TYPE_MOTION);
        event.timestamp = timestamp;
        event.action = action;
        event.x = clamp01(x);
        event.y = clamp01(y);
        event.hasPointer = havePointer;
        return event;
    }

    private static float[] parsePointer(byte[] bytes) {
        ProtoReader reader = new ProtoReader(bytes);
        float x = 0.0f;
        float y = 0.0f;

        while (reader.hasRemaining()) {
            int tag = reader.readVarint32();
            int field = tag >>> 3;
            int wire = tag & 7;

            if (field == 2 && wire == 5) {
                x = reader.readFloat();
            } else if (field == 3 && wire == 5) {
                y = reader.readFloat();
            } else {
                reader.skip(wire);
            }
        }

        return new float[] {x, y};
    }

    private static Event parseVector(
        int type,
        byte[] bytes,
        boolean orientation
    ) {
        if (bytes == null) {
            return Event.unknown(type);
        }

        ProtoReader reader = new ProtoReader(bytes);
        Event event = new Event(type);

        while (reader.hasRemaining()) {
            int tag = reader.readVarint32();
            int field = tag >>> 3;
            int wire = tag & 7;

            if (field == 1 && wire == 0) {
                event.timestamp = reader.readVarint64();
            } else if (field >= 2 && field <= (orientation ? 5 : 4) && wire == 5) {
                float value = reader.readFloat();
                if (field == 2) {
                    event.x = value;
                } else if (field == 3) {
                    event.y = value;
                } else if (field == 4) {
                    event.z = value;
                } else if (field == 5) {
                    event.w = value;
                }
            } else {
                reader.skip(wire);
            }
        }

        return event;
    }

    private static Event parseKey(byte[] bytes) {
        if (bytes == null) {
            return Event.unknown(TYPE_KEY);
        }

        ProtoReader reader = new ProtoReader(bytes);
        Event event = new Event(TYPE_KEY);

        while (reader.hasRemaining()) {
            int tag = reader.readVarint32();
            int field = tag >>> 3;
            int wire = tag & 7;

            if (field == 1 && wire == 0) {
                event.action = reader.readVarint32();
            } else if (field == 2 && wire == 0) {
                event.keyCode = reader.readVarint32();
            } else {
                reader.skip(wire);
            }
        }

        return event;
    }

    private static float clamp01(float value) {
        return Math.max(0.0f, Math.min(1.0f, value));
    }

    static final class Event {
        final int type;
        long timestamp;
        int action;
        int keyCode;
        float x;
        float y;
        float z;
        float w = 1.0f;
        boolean hasPointer;

        Event(int type) {
            this.type = type;
        }

        static Event unknown(int type) {
            return new Event(type);
        }
    }

    private static final class ProtoReader {
        private final byte[] data;
        private int position;

        ProtoReader(byte[] data) {
            this.data = data == null ? new byte[0] : data;
        }

        boolean hasRemaining() {
            return position < data.length;
        }

        int readVarint32() {
            long value = readVarint64();
            if (value > Integer.MAX_VALUE) {
                return (int) value;
            }
            return (int) value;
        }

        long readVarint64() {
            long result = 0L;
            int shift = 0;

            while (shift < 64) {
                require(1);
                int value = data[position++] & 0xff;
                result |= (long) (value & 0x7f) << shift;
                if ((value & 0x80) == 0) {
                    return result;
                }
                shift += 7;
            }

            throw new IllegalArgumentException("Malformed protobuf varint.");
        }

        float readFloat() {
            require(4);
            int bits =
                (data[position] & 0xff)
                    | ((data[position + 1] & 0xff) << 8)
                    | ((data[position + 2] & 0xff) << 16)
                    | ((data[position + 3] & 0xff) << 24);
            position += 4;
            return Float.intBitsToFloat(bits);
        }

        byte[] readBytes() {
            int length = readVarint32();
            if (length < 0) {
                throw new IllegalArgumentException("Negative protobuf field length.");
            }
            require(length);
            byte[] result = Arrays.copyOfRange(
                data,
                position,
                position + length
            );
            position += length;
            return result;
        }

        void skip(int wireType) {
            switch (wireType) {
                case 0:
                    readVarint64();
                    return;
                case 1:
                    require(8);
                    position += 8;
                    return;
                case 2:
                    int length = readVarint32();
                    require(length);
                    position += length;
                    return;
                case 5:
                    require(4);
                    position += 4;
                    return;
                default:
                    throw new IllegalArgumentException(
                        "Unsupported protobuf wire type: " + wireType
                    );
            }
        }

        private void require(int count) {
            if (count < 0 || position + count > data.length) {
                throw new IllegalArgumentException(
                    "Controller emulator protobuf message ended unexpectedly."
                );
            }
        }
    }
}
