package io.github.mrcalzon02.reverievr;

import java.util.Locale;

final class ControllerSnapshot {
    final long receivedAtNanos;
    final int timestampMillisModulo512;
    final int packetIndex;

    final float orientationX;
    final float orientationY;
    final float orientationZ;
    final float orientationW;

    final float accelXG;
    final float accelYG;
    final float accelZG;

    final float gyroXDps;
    final float gyroYDps;
    final float gyroZDps;

    final boolean touching;
    final int touchX;
    final int touchY;

    final boolean volumeUpPressed;
    final boolean volumeDownPressed;
    final boolean menuPressed;
    final boolean homePressed;
    final boolean touchpadPressed;

    final int warningBits;

    ControllerSnapshot(
        long receivedAtNanos,
        int timestampMillisModulo512,
        int packetIndex,
        float orientationX,
        float orientationY,
        float orientationZ,
        float orientationW,
        float accelXG,
        float accelYG,
        float accelZG,
        float gyroXDps,
        float gyroYDps,
        float gyroZDps,
        boolean touching,
        int touchX,
        int touchY,
        boolean volumeUpPressed,
        boolean volumeDownPressed,
        boolean menuPressed,
        boolean homePressed,
        boolean touchpadPressed,
        int warningBits
    ) {
        this.receivedAtNanos = receivedAtNanos;
        this.timestampMillisModulo512 = timestampMillisModulo512;
        this.packetIndex = packetIndex;
        this.orientationX = orientationX;
        this.orientationY = orientationY;
        this.orientationZ = orientationZ;
        this.orientationW = orientationW;
        this.accelXG = accelXG;
        this.accelYG = accelYG;
        this.accelZG = accelZG;
        this.gyroXDps = gyroXDps;
        this.gyroYDps = gyroYDps;
        this.gyroZDps = gyroZDps;
        this.touching = touching;
        this.touchX = touchX;
        this.touchY = touchY;
        this.volumeUpPressed = volumeUpPressed;
        this.volumeDownPressed = volumeDownPressed;
        this.menuPressed = menuPressed;
        this.homePressed = homePressed;
        this.touchpadPressed = touchpadPressed;
        this.warningBits = warningBits;
    }

    String toDiagnosticString() {
        String touch = touching
            ? String.format(Locale.US, "%d,%d", touchX, touchY)
            : "--";

        return String.format(
            Locale.US,
            "Pose q(%.3f, %.3f, %.3f, %.3f)\n"
                + "Touch %s  •  click:%s  menu:%s  home:%s\n"
                + "Vol +:%s  -:%s  •  pkt:%d  t:%dms",
            orientationX,
            orientationY,
            orientationZ,
            orientationW,
            touch,
            yesNo(touchpadPressed),
            yesNo(menuPressed),
            yesNo(homePressed),
            yesNo(volumeUpPressed),
            yesNo(volumeDownPressed),
            packetIndex,
            timestampMillisModulo512
        );
    }

    private static String yesNo(boolean value) {
        return value ? "1" : "0";
    }
}
