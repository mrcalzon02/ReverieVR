package io.github.mrcalzon02.reverievr;

/** Independently testable VR policy; Android event classification stays in VrActivity. */
final class ExternalHidInputPolicy {
    private ExternalHidInputPolicy() {}

    static boolean shouldConsume(boolean enabled, boolean keyboard, boolean mouse,
                                 boolean gamepad, boolean recovery) {
        return !enabled && !recovery && !gamepad && (keyboard || mouse);
    }
}
