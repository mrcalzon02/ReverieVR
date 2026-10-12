package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

final class BuiltInBindingProfiles {
    static final String ID_NONE = "none";
    static final String ID_DOS_DOOM_SHAREWARE = "dos-doom-shareware";
    static final String ID_DOS_FPS_HEAD_MOUSE = "dos-fps-head-mouse";
    static final String ID_DOS_CURSOR = "dos-cursor";
    static final String ID_NATIVE_STANDARD =
        "native-standard";

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
                && !ID_NONE.equals(profile.id)) {
                result.add(profile);
            }
        }
        return Collections.unmodifiableList(result);
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

        List<InputBinding> doom = new ArrayList<>();
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
                650.0f,
                0.0f
            )
        );
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
        doom.add(
            InputBinding.positiveThreshold(
                BindingInput.TOUCHPAD_Y,
                VirtualOutput.key(VirtualKey.DOWN),
                0.35f
            )
        );
        doom.add(
            InputBinding.negativeThreshold(
                BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.LEFT),
                0.35f
            )
        );
        doom.add(
            InputBinding.positiveThreshold(
                BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.RIGHT),
                0.35f
            )
        );
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
        cursor.add(
            InputBinding.digital(
                BindingInput.SELECT,
                VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)
            )
        );
        profiles.add(
            new BindingProfile(
                ID_DOS_CURSOR,
                "DOS mouse — touchpad movement / click",
                cursor
            )
        );

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

        return Collections.unmodifiableList(profiles);
    }
}
