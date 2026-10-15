package io.github.mrcalzon02.reverievr;

enum BindingInput {
    SELECT,
    // Daydream App/minus is distinct from Home and the volume rocker.
    DAYDREAM_APP_BUTTON,
    BACK,
    RECENTER,
    VOLUME_UP,
    VOLUME_DOWN,

    NAV_LEFT,
    NAV_RIGHT,
    NAV_UP,
    NAV_DOWN,

    TOUCHPAD_X,
    TOUCHPAD_Y,
    TOUCHPAD_REL_X,
    TOUCHPAD_REL_Y,

    GAMEPAD_X,
    GAMEPAD_Y,
    GAMEPAD_RX,
    GAMEPAD_RY,

    HEAD_YAW_DELTA,
    HEAD_PITCH_DELTA,
    // Head forward vector Y, positive for looking up. Absolute per-frame
    // value, unlike the moving-only pitch delta used by mouse profiles.
    HEAD_PITCH_UP_AXIS,

    // Physical Android HID button IDs, independent of the labels on
    // generic / Xbox 360 / DualShock faceplates. The shell still reserves
    // platform Menu/recovery when that source sends it.
    GAMEPAD_BUTTON_A,
    GAMEPAD_BUTTON_B,
    GAMEPAD_BUTTON_X,
    GAMEPAD_BUTTON_Y,
    GAMEPAD_BUTTON_L1,
    GAMEPAD_BUTTON_R1,
    GAMEPAD_BUTTON_L2,
    GAMEPAD_BUTTON_R2,
    GAMEPAD_BUTTON_THUMBL,
    GAMEPAD_BUTTON_THUMBR,
    GAMEPAD_BUTTON_SELECT,
    GAMEPAD_BUTTON_START,
    GAMEPAD_BUTTON_MODE,
    GAMEPAD_DPAD_LEFT,
    GAMEPAD_DPAD_RIGHT,
    GAMEPAD_DPAD_UP,
    GAMEPAD_DPAD_DOWN,
    GAMEPAD_TRIGGER_LEFT,
    GAMEPAD_TRIGGER_RIGHT,
    CONTROLLER_YAW_DELTA,
    CONTROLLER_PITCH_DELTA,
    DAYDREAM_TAP,
    DAYDREAM_DOUBLE_TAP,
    DAYDREAM_PRESS_HOLD,
    DAYDREAM_SHAKE,
    HEAD_LOOK_EXTREME_UP,
    HEAD_LOOK_EXTREME_DOWN,
    HEAD_SHARP_LEFT,
    HEAD_SHARP_RIGHT,
    HEAD_ROLL_POSITIVE,
    HEAD_ROLL_NEGATIVE
}
