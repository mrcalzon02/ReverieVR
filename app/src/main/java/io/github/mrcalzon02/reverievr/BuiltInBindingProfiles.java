package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

final class BuiltInBindingProfiles {
    static final String ID_NONE = "none";
    static final String ID_DOS_STANDARD = "dos-standard";
    static final String ID_DOS_DOOM_SHAREWARE = "dos-doom-shareware";
    static final String ID_DOS_FPS_HEAD_MOUSE = "dos-fps-head-mouse";
    static final String ID_DOS_CURSOR = "dos-cursor";
    static final String ID_DOS_HEAD_MOUSE = "dos-head-mouse";
    static final String ID_DOS_CONTROLLER_MOUSE = "dos-controller-mouse";
    static final String ID_DOS_TOUCHPAD_MOUSE = "dos-touchpad-mouse";
    static final String ID_NATIVE_STANDARD =
        "native-standard";
    static final String ID_NATIVE_CLASSICS =
        "native-classics";

    static String nativeProfileForModule(String moduleId) {
        // Board cursor, Snake direction and Pong paddle need the Daydream
        // touchpad axes; immersive worlds must retain shell-owned locomotion.
        return moduleId != null && moduleId.startsWith("classic-")
            ? ID_NATIVE_CLASSICS : ID_NATIVE_STANDARD;
    }

    private static final List<BindingProfile> PROFILES =
        buildProfiles();
    private static final List<BindingProfile> DOS_PROFILES =
        buildDosProfiles();

    private BuiltInBindingProfiles() {
    }

    static List<BindingProfile> all() {
        return PROFILES;
    }

    static BindingProfile byId(String id) {
        if (id != null) {
            for (BindingProfile profile : PROFILES) {
                if (profile.id.equals(id)) {
                    return profile;
                }
            }
        }
        return PROFILES.get(0);
    }

    static List<BindingProfile> dosProfiles() {
        return DOS_PROFILES;
    }

    static boolean isDosProfileId(String id) {
        if (id == null) {
            return false;
        }
        for (BindingProfile profile : DOS_PROFILES) {
            if (profile.id.equals(id)) {
                return true;
            }
        }
        return false;
    }

    private static List<BindingProfile> buildDosProfiles() {
        List<BindingProfile> result = new ArrayList<>();
        for (BindingProfile profile : PROFILES) {
            if (!ID_NATIVE_STANDARD.equals(profile.id)
                && !ID_NATIVE_CLASSICS.equals(profile.id)
                && !ID_NONE.equals(profile.id)) {
                result.add(profile);
            }
        }
        return Collections.unmodifiableList(result);
    }

    /** DOSBox Pure uses a libretro joypad for its own game chooser,
     * while installed DOS programs also require real keyboard events. */
    private static void addDosMenuBindings(List<InputBinding> bindings) {
        bindings.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.joystickButton(0)));
        bindings.add(InputBinding.digital(BindingInput.VOLUME_UP,
            VirtualOutput.key(VirtualKey.ENTER)));
        bindings.add(InputBinding.digital(BindingInput.VOLUME_DOWN,
            VirtualOutput.key(VirtualKey.ESCAPE)));
        // Android / Steam gamepad D-pad works in DOS even without pad touch.
        bindings.add(InputBinding.digital(BindingInput.GAMEPAD_DPAD_UP,
            VirtualOutput.key(VirtualKey.UP)));
        bindings.add(InputBinding.digital(BindingInput.GAMEPAD_DPAD_DOWN,
            VirtualOutput.key(VirtualKey.DOWN)));
        bindings.add(InputBinding.digital(BindingInput.GAMEPAD_DPAD_LEFT,
            VirtualOutput.key(VirtualKey.LEFT)));
        bindings.add(InputBinding.digital(BindingInput.GAMEPAD_DPAD_RIGHT,
            VirtualOutput.key(VirtualKey.RIGHT)));
        bindings.add(InputBinding.analog(BindingInput.GAMEPAD_X,
            VirtualOutput.joystickAxisX(), 1.0f, 0.15f));
        bindings.add(InputBinding.analog(BindingInput.GAMEPAD_Y,
            VirtualOutput.joystickAxisY(), 1.0f, 0.15f));
    }

    private static void addDosTouchpadArrows(List<InputBinding> bindings) {
        BindingInput[] axes = {BindingInput.TOUCHPAD_Y, BindingInput.TOUCHPAD_Y,
            BindingInput.TOUCHPAD_X, BindingInput.TOUCHPAD_X};
        VirtualKey[] keys = {VirtualKey.UP, VirtualKey.DOWN,
            VirtualKey.LEFT, VirtualKey.RIGHT};
        for (int i = 0; i < axes.length; i++) {
            boolean negative = i == 0 || i == 2;
            bindings.add(negative
                ? InputBinding.negativeThreshold(axes[i],
                    VirtualOutput.key(keys[i]), 0.35f)
                : InputBinding.positiveThreshold(axes[i],
                    VirtualOutput.key(keys[i]), 0.35f));
            bindings.add(negative
                ? InputBinding.negativeThreshold(axes[i],
                    VirtualOutput.joystickButton(4 + i), 0.35f)
                : InputBinding.positiveThreshold(axes[i],
                    VirtualOutput.joystickButton(4 + i), 0.35f));
        }
    }

    private static List<BindingProfile> buildProfiles() {
        List<BindingProfile> profiles = new ArrayList<>();

        profiles.add(
            new BindingProfile(
                ID_NONE,
                "No hosted-game bindings",
                Collections.emptyList()
            )
        );

        // Functional import default: arrows on Daydream pad, mouse at gaze,
        // trackpad click as mouse button, rocker for Enter/Escape.
        List<InputBinding> standard = new ArrayList<>();
        standard.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)));
        standard.add(InputBinding.analog(BindingInput.HEAD_YAW_DELTA,
            VirtualOutput.mouseRelativeX(), 650.0f, 0.0f));
        standard.add(InputBinding.analog(BindingInput.HEAD_PITCH_DELTA,
            VirtualOutput.mouseRelativeY(), -650.0f, 0.0f));
        BindingInput[] axes = { BindingInput.TOUCHPAD_Y, BindingInput.TOUCHPAD_Y,
            BindingInput.TOUCHPAD_X, BindingInput.TOUCHPAD_X };
        VirtualKey[] keyDirections = { VirtualKey.UP, VirtualKey.DOWN,
            VirtualKey.LEFT, VirtualKey.RIGHT };
        for (int i = 0; i < axes.length; i++) {
            standard.add(i == 0 || i == 2
                ? InputBinding.negativeThreshold(axes[i],
                    VirtualOutput.key(keyDirections[i]), 0.35f)
                : InputBinding.positiveThreshold(axes[i],
                    VirtualOutput.key(keyDirections[i]), 0.35f));
            standard.add(i == 0 || i == 2
                ? InputBinding.negativeThreshold(axes[i],
                    VirtualOutput.joystickButton(4+i), 0.35f)
                : InputBinding.positiveThreshold(axes[i],
                    VirtualOutput.joystickButton(4+i), 0.35f));
        }
        addDosMenuBindings(standard);
        profiles.add(new BindingProfile(ID_DOS_STANDARD,
            "DOS standard - pad arrows / head mouse", standard));

        List<InputBinding> doom = new ArrayList<>();
        // Reuse headset-view yaw polarity for the DOS guest pointer.
        // The opposite sign turns looking left into moving right.
        doom.add(
            InputBinding.digital(
                BindingInput.SELECT,
                VirtualOutput.key(VirtualKey.CTRL)
            )
        );
        doom.add(
            InputBinding.analog(
                BindingInput.HEAD_YAW_DELTA,
                VirtualOutput.mouseRelativeX(),
                -650.0f,
                0.0f
            )
        );
        // Original DOS Doom has NO free vertical camera pitch. Use head
        // inclination as actual Up/Down guest keyboard controls instead:
        // in-game Forward/Back, and UP/DOWN navigation in Doom menus.
        // Entry at sin(12 deg)~.21; InputBindingEngine releases at 70%
        // of that angle threshold to prevent neutral-zone chatter.
        doom.add(InputBinding.positiveThreshold(
            BindingInput.HEAD_PITCH_UP_AXIS,
            VirtualOutput.key(VirtualKey.UP), 0.21f));
        doom.add(InputBinding.negativeThreshold(
            BindingInput.HEAD_PITCH_UP_AXIS,
            VirtualOutput.key(VirtualKey.DOWN), 0.21f));
        // Doom can fire on either its keyboard default or DOS mouse button.
        doom.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)));
        doom.add(
            InputBinding.negativeThreshold(
                BindingInput.TOUCHPAD_Y,
                VirtualOutput.key(VirtualKey.UP),
                0.35f
            )
        );
        doom.add(InputBinding.negativeThreshold(BindingInput.TOUCHPAD_Y,
            VirtualOutput.joystickButton(4), 0.35f));
        doom.add(
            InputBinding.positiveThreshold(
                BindingInput.TOUCHPAD_Y,
                VirtualOutput.key(VirtualKey.DOWN),
                0.35f
            )
        );
        doom.add(InputBinding.positiveThreshold(BindingInput.TOUCHPAD_Y,
            VirtualOutput.joystickButton(5), 0.35f));
        doom.add(
            InputBinding.negativeThreshold(
                BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.LEFT),
                0.35f
            )
        );
        doom.add(InputBinding.negativeThreshold(BindingInput.TOUCHPAD_X,
            VirtualOutput.joystickButton(6), 0.35f));
        doom.add(
            InputBinding.positiveThreshold(
                BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.RIGHT),
                0.35f
            )
        );
        doom.add(InputBinding.positiveThreshold(BindingInput.TOUCHPAD_X,
            VirtualOutput.joystickButton(7), 0.35f));
        addDosMenuBindings(doom);
        profiles.add(
            new BindingProfile(
                ID_DOS_DOOM_SHAREWARE,
                "DOS Doom shareware — head turn",
                doom
            )
        );

        List<InputBinding> fps = new ArrayList<>();
        fps.add(
            InputBinding.digital(
                BindingInput.SELECT,
                VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)
            )
        );
        fps.add(
            InputBinding.analog(
                BindingInput.HEAD_YAW_DELTA,
                VirtualOutput.mouseRelativeX(),
                650.0f,
                0.0f
            )
        );
        fps.add(
            InputBinding.analog(
                BindingInput.HEAD_PITCH_DELTA,
                VirtualOutput.mouseRelativeY(),
                -650.0f,
                0.0f
            )
        );
        fps.add(
            InputBinding.negativeThreshold(
                BindingInput.TOUCHPAD_Y,
                VirtualOutput.key(VirtualKey.W),
                0.35f
            )
        );
        fps.add(
            InputBinding.positiveThreshold(
                BindingInput.TOUCHPAD_Y,
                VirtualOutput.key(VirtualKey.S),
                0.35f
            )
        );
        fps.add(
            InputBinding.negativeThreshold(
                BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.A),
                0.35f
            )
        );
        fps.add(
            InputBinding.positiveThreshold(
                BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.D),
                0.35f
            )
        );
        addDosMenuBindings(fps);
        profiles.add(
            new BindingProfile(
                ID_DOS_FPS_HEAD_MOUSE,
                "DOS FPS — head mouse look",
                fps
            )
        );

        List<InputBinding> cursor = new ArrayList<>();
        cursor.add(InputBinding.analog(
            BindingInput.TOUCHPAD_REL_X, VirtualOutput.mouseRelativeX(),
            0.70f, 0.0f));
        cursor.add(InputBinding.analog(
            BindingInput.TOUCHPAD_REL_Y, VirtualOutput.mouseRelativeY(),
            0.70f, 0.0f));
        // Match the verified DOS headset-mouse yaw mapping.
        cursor.add(InputBinding.analog(BindingInput.HEAD_YAW_DELTA,
            VirtualOutput.mouseRelativeX(), -650.0f, 0.0f));
        cursor.add(InputBinding.analog(BindingInput.HEAD_PITCH_DELTA,
            VirtualOutput.mouseRelativeY(), -650.0f, 0.0f));
        cursor.add(
            InputBinding.digital(
                BindingInput.SELECT,
                VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)
            )
        );
        addDosMenuBindings(cursor);
        profiles.add(
            new BindingProfile(
                ID_DOS_CURSOR,
                "DOS pointer — head gaze + touchpad click",
                cursor
            )
        );

        // Independent DOS cursor sources. Only the active profile drives
        // the guest pointer; Select remains left click in each mode.
        List<InputBinding> headMouse = new ArrayList<>();
        // The Cardboard view Euler deltas have the opposite pointer polarity
        // to the DOS guest's horizontal/right and vertical/down mouse axes.
        // Correct this binding, not the physical headset view matrices.
        headMouse.add(InputBinding.analog(BindingInput.HEAD_YAW_DELTA,
            VirtualOutput.mouseRelativeX(), -650.0f, 0.0f));
        headMouse.add(InputBinding.analog(BindingInput.HEAD_PITCH_DELTA,
            VirtualOutput.mouseRelativeY(), 650.0f, 0.0f));
        headMouse.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)));
        addDosTouchpadArrows(headMouse);
        addDosMenuBindings(headMouse);
        profiles.add(new BindingProfile(ID_DOS_HEAD_MOUSE,
            "DOS mouse - headset view", headMouse));

        List<InputBinding> controllerMouse = new ArrayList<>();
        controllerMouse.add(InputBinding.analog(BindingInput.CONTROLLER_YAW_DELTA,
            VirtualOutput.mouseRelativeX(), 650.0f, 0.0f));
        controllerMouse.add(InputBinding.analog(BindingInput.CONTROLLER_PITCH_DELTA,
            VirtualOutput.mouseRelativeY(), -650.0f, 0.0f));
        controllerMouse.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)));
        addDosTouchpadArrows(controllerMouse);
        addDosMenuBindings(controllerMouse);
        profiles.add(new BindingProfile(ID_DOS_CONTROLLER_MOUSE,
            "DOS mouse - controller rotation", controllerMouse));

        List<InputBinding> padMouse = new ArrayList<>();
        padMouse.add(InputBinding.analog(BindingInput.TOUCHPAD_REL_X,
            VirtualOutput.mouseRelativeX(), 0.70f, 0.0f));
        padMouse.add(InputBinding.analog(BindingInput.TOUCHPAD_REL_Y,
            VirtualOutput.mouseRelativeY(), 0.70f, 0.0f));
        padMouse.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)));
        addDosMenuBindings(padMouse);
        profiles.add(new BindingProfile(ID_DOS_TOUCHPAD_MOUSE,
            "DOS mouse - touchpad swipe", padMouse));

        List<InputBinding> nativeStandard =
            new ArrayList<>();
        // Native 3D locomotion is shell-owned; do not move twice.
        nativeStandard.add(
            InputBinding.digital(
                BindingInput.SELECT,
                VirtualOutput.joystickButton(0)
            )
        );
        nativeStandard.add(
            InputBinding.digital(
                BindingInput.VOLUME_UP,
                VirtualOutput.joystickButton(1)
            )
        );
        nativeStandard.add(
            InputBinding.digital(
                BindingInput.VOLUME_DOWN,
                VirtualOutput.joystickButton(2)
            )
        );
        // These gamepad mappings are normalized Android HID sources, not
        // per-game vendor-code switches. A still uses SELECT -> button 0;
        // Physical B is a hosted game button in the active native/DOS
        // experience; shell Menu and Android Back remain recoverable.
        BindingInput[] physicalInputs = {
            BindingInput.GAMEPAD_BUTTON_B,
            BindingInput.GAMEPAD_BUTTON_X,
            BindingInput.GAMEPAD_BUTTON_Y,
            BindingInput.GAMEPAD_BUTTON_L1,
            BindingInput.GAMEPAD_BUTTON_R1,
            BindingInput.GAMEPAD_BUTTON_L2,
            BindingInput.GAMEPAD_BUTTON_R2,
            BindingInput.GAMEPAD_BUTTON_THUMBL,
            BindingInput.GAMEPAD_BUTTON_THUMBR,
            BindingInput.GAMEPAD_BUTTON_SELECT
        };
        for (int i=0; i<physicalInputs.length; i++) {
            nativeStandard.add(InputBinding.digital(
                physicalInputs[i], VirtualOutput.joystickButton(3+i)));
        }
        // Native ABI has joystick buttons 13-15 free. Distinct Daydream
        // gestures are opt-in secondary actions; primary SELECT still works
        // immediately and platform Menu/Home remain reserved for recovery.
        nativeStandard.add(InputBinding.digital(
            BindingInput.DAYDREAM_TAP, VirtualOutput.joystickButton(13)));
        nativeStandard.add(InputBinding.digital(
            BindingInput.DAYDREAM_DOUBLE_TAP, VirtualOutput.joystickButton(14)));
        nativeStandard.add(InputBinding.digital(
            BindingInput.DAYDREAM_PRESS_HOLD, VirtualOutput.joystickButton(15)));
        // The native game's movement axes are distinct from Daydream's
        // shell-owned locomotion. Modules choose whether to consume them.
        nativeStandard.add(InputBinding.analog(
            BindingInput.GAMEPAD_X, VirtualOutput.joystickAxisX(), 1.0f, 0.15f));
        nativeStandard.add(InputBinding.analog(
            BindingInput.GAMEPAD_Y, VirtualOutput.joystickAxisY(), 1.0f, 0.15f));
        profiles.add(
            new BindingProfile(
                ID_NATIVE_STANDARD,
                "Native game — standard controls",
                nativeStandard
            )
        );

        // Classic games have no shell-travel capability. Reuse all native
        // Select, rocker and HID actions, adding only Daydream's centered
        // analog pad for playable paddle/direction/board-cursor movement.
        // Never change the input policy of world-scale native games.
        List<InputBinding> nativeClassics =
            new ArrayList<>(nativeStandard);
        nativeClassics.add(InputBinding.analog(
            BindingInput.TOUCHPAD_X,
            VirtualOutput.joystickAxisX(), 1.0f, 0.28f));
        nativeClassics.add(InputBinding.analog(
            BindingInput.TOUCHPAD_Y,
            VirtualOutput.joystickAxisY(), 1.0f, 0.28f));
        profiles.add(new BindingProfile(
            ID_NATIVE_CLASSICS,
            "Native classic games — touchpad movement and Select",
            nativeClassics
        ));

        return Collections.unmodifiableList(profiles);
    }
}
