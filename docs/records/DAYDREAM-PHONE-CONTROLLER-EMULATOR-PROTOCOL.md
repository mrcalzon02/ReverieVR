# Historical Daydream Phone Controller Emulator Protocol

**Purpose:** interoperability research record  
**Status:** draft until second-phone hardware validation  
**Date:** 2026-10-05

## User-visible historical behavior

Google's deprecated Daydream Controller Emulator allowed a second Android phone with a gyroscope to stand in for the physical Daydream controller.

The headset phone and controller phone were paired through Android Bluetooth. Google VR Services then exposed a developer setting for choosing the controller-emulator device.

The emulator surface provided:

- phone orientation as 3DoF controller orientation;
- touchpad touch/motion;
- touchpad click;
- App button;
- Home/recenter.

Google's documentation notes that the emulator did not emulate the physical controller's volume buttons.

## Headset-side interoperability target

Public historical compatibility behavior identifies the classic Bluetooth RFCOMM service UUID as:

`ab001ac1-d740-4abb-a8e6-1cb5a49628fa`

The headset is the RFCOMM client.

Messages are framed as:

1. 4-byte big-endian payload length;
2. protobuf-wire payload.

The top-level payload contains a type plus one nested event.

Observed event families relevant to ReverieVR:

- Motion = 1
- Gyroscope = 2
- Accelerometer = 3
- Orientation = 5
- Key = 6

Relevant historical key codes:

- Home = `0x03`
- Volume Up = `0x18`
- Volume Down = `0x19`
- Click = `0x42`
- App = `0x52`

ReverieVR parses only the protobuf wire types and fields required for these events. It does not embed generated protobuf source from the historical project.

## Coordinate compatibility

For interoperability with historical Daydream controller-client behavior, phone orientation is normalized at the provider boundary before it reaches ReverieVR's controller snapshot/action layer.

Exact handedness, touch-axis orientation, and recenter comfort remain device-validation items; do not silently change them merely to make a desktop test look plausible.

## Security/recovery boundary

ReverieVR connects only after the user explicitly chooses an already-paired Bluetooth device from Stage A.

The phone-controller path does not scan arbitrary nearby classic Bluetooth devices and does not make the headset phone an open RFCOMM server.

A failed or malformed stream must produce a visible error/disconnect state and must not block the physical Daydream or Android-gamepad recovery paths.
