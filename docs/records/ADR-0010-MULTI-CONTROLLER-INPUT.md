# ADR-0010 — Multiple Controller Transports, One VR Action Layer

**Status:** Accepted as implementation baseline  
**Date:** 2026-10-05

## Context

ReverieVR's reference controller is the physical Daydream 3DoF handset, but useful revival hardware should not become unusable merely because that one controller is unavailable, discharged, or lost.

Google historically shipped a Controller Emulator that allowed a second Android phone with a gyroscope to act as a Daydream controller. That path paired the phones over Bluetooth and was selected as a controller-emulator device in Google VR Services.

Separately, Android already exposes ordinary Bluetooth and USB gamepads through the platform input-device APIs.

Those sources do not share one transport:

- the physical Daydream controller uses BLE/GATT;
- the historical phone emulator uses a paired classic-Bluetooth RFCOMM stream;
- Android gamepads appear as Android input devices.

Binding the VR shell directly to any one transport would make later compatibility work invasive.

## Decision

ReverieVR separates **transport state** from **VR actions**.

### Supported draft sources

1. **Physical Daydream controller**
   - BLE FE55/GATT transport;
   - pose/touch/button packet decoder;
   - controller battery/voltage when available;
   - hardware recenter command.

2. **Historical Daydream phone-controller emulator compatibility**
   - second phone is paired through normal Android Bluetooth settings;
   - headset connects to RFCOMM service UUID `ab001ac1-d740-4abb-a8e6-1cb5a49628fa`;
   - stream framing is a 4-byte big-endian payload length followed by one protobuf-wire `PhoneEvent`;
   - decoded event families are motion/touch, gyro, accelerometer, orientation, and key;
   - battery remains unknown because this transport does not provide reliable remote-phone battery telemetry.

3. **Android gamepad/joystick**
   - no ReverieVR-specific Bluetooth protocol;
   - Android `InputDevice`, `KeyEvent`, and `MotionEvent` remain authoritative;
   - attach/remove/change state updates Stage A/Stage B readiness live.

### Normalized action layer

Transport-specific state feeds `VrInputRouter`, which emits named `VrInputAction` values:

- SELECT;
- BACK;
- RECENTER;
- NAV_LEFT / NAV_RIGHT / NAV_UP / NAV_DOWN;
- VOLUME_UP / VOLUME_DOWN.

The VR activity consumes these names rather than physical Daydream bit positions or Android key codes.

Raw pose/touch/sensor data may still be retained for diagnostics and future pointer/gesture features; it does not redefine platform action semantics.

## Phone-emulator provenance

The deprecated Google documentation establishes that a second Android phone could emulate a Daydream controller over a paired Bluetooth relationship.

The RFCOMM/framing details were researched from public compatibility behavior and the historical `domination/gvr-services-emulator` project at commit `9be3bb01701de49bdc52cccea4706909f557f42a`.

That repository declares no license in GitHub metadata. ReverieVR therefore does **not** copy or import its Java/protobuf implementation. ReverieVR contains an independently authored minimal wire decoder based on the protocol field/framing facts needed for interoperability.

ReverieVR does not redistribute Google's deprecated Controller Emulator APK.

## Future companion application

RV-0094 reserves an independently authored **ReverieVR Controller** companion APK for a spare Android phone.

The intended UI is deliberately close to the physical-controller interaction model—orientation from phone sensors, one touchpad surface, Select, App/Back, Home/Recenter—without requiring Google VR Services.

The headset-side historical compatibility receiver should be validated before that companion app is considered stable.

## Acceptance

This architecture remains device-draft until:

1. physical Daydream input is validated;
2. a compatible second-phone emulator connects over RFCOMM and produces correct orientation/touch/button behavior;
3. at least one ordinary Android Bluetooth/USB gamepad can enter and operate Stage B;
4. source switching does not strand the user;
5. generic/phone-emulator sources report unknown battery rather than fabricated percentages;
6. input axis/polarity and recenter behavior are verified on the Galaxy S9.
