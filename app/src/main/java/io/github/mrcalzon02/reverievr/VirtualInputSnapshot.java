package io.github.mrcalzon02.reverievr;

final class VirtualInputSnapshot {
    final boolean[] keys =
        new boolean[VirtualKey.values().length];
    final boolean[] mouseButtons = new boolean[3];
    final boolean[] joystickButtons =
        new boolean[VirtualInputBus.MAX_JOYSTICK_BUTTONS];

    float mouseRelativeX;
    float mouseRelativeY;
    float mouseAbsoluteX;
    float mouseAbsoluteY;
    float mouseWheel;
    float joystickAxisX;
    float joystickAxisY;
    long generation;
}
