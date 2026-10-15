package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

final class BuiltInBindingProfiles {
    static final String ID_NONE = "none";
    static final String ID_DOS_STANDARD = "dos-standard";
    static final String ID_DOS_DOOM_SHAREWARE = "dos-doom-shareware";
    static final String ID_DOS_KENS_LABYRINTH = "dos-kens-labyrinth";
    static final String ID_DOS_JETPACK = "dos-jetpack";
    static final String ID_DOS_SQUAREZ = "dos-squarez";
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

    /**
     * Refresh ONLY an untouched copy of one of the obsolete shipped
     * headset-mouse presets. Previous releases saved these full presets
     * in module-local RVBIND1 files; changing the built-in defaults alone
     * cannot fix such an existing installation.
     *
     * Do not rewrite any tuned deadzone, sensitivity, command or layout.
     * Explicit user-customized configurations continue to take precedence.
     */
    static BindingProfile upgradeLegacyStockHeadMouse(BindingProfile saved) {
        if (saved == null) return null;
        final boolean oldHead =
            ID_DOS_HEAD_MOUSE.equals(saved.id);
        final boolean oldDoom =
            ID_DOS_DOOM_SHAREWARE.equals(saved.id);
        final boolean oldCursor =
            ID_DOS_CURSOR.equals(saved.id);
        if (!oldHead && !oldDoom && !oldCursor) return saved;

        final BindingProfile current = byId(saved.id);
        if (oldDoom && isUnchangedLegacyDoomLayout(saved, current))
            return current;
        if (!current.displayName.equals(saved.displayName)
            || current.bindings.size() != saved.bindings.size())
            return saved;

        int oldSigns = 0;
        for (int i=0; i<saved.bindings.size(); i++) {
            final InputBinding before = saved.bindings.get(i);
            final InputBinding now = current.bindings.get(i);
            if (before.input != now.input
                || !before.output.equals(now.output)
                || before.transform != now.transform
                || before.deadzone != now.deadzone
                || before.threshold != now.threshold)
                return saved;
            if (before.scale == now.scale) continue;
            final boolean legacyYaw =
                before.input == BindingInput.HEAD_YAW_DELTA
                && before.output.equals(VirtualOutput.mouseRelativeX());
            final boolean legacyPitch = oldHead
                && before.input == BindingInput.HEAD_PITCH_DELTA
                && before.output.equals(VirtualOutput.mouseRelativeY());
            if ((!legacyYaw && !legacyPitch)
                || before.scale != -now.scale
                || Math.abs(now.scale) != 650.0f)
                return saved;
            oldSigns++;
        }
        return oldSigns == (oldHead ? 2 : 1) ? current : saved;
    }

    /** Match ONLY the shipped Doom layout from before App/right-click and
     * rocker weapon cycling. Never discard user-tuned or remapped bindings. */
    private static boolean isUnchangedLegacyDoomLayout(
        BindingProfile saved, BindingProfile current) {
        if (!current.displayName.equals(saved.displayName)) return false;
        List<InputBinding> expected = new ArrayList<>();
        boolean hasJoySelect = false;
        for (InputBinding now : current.bindings) {
            if (now.input == BindingInput.DAYDREAM_APP_BUTTON) continue;
            expected.add(now);
            if (now.input == BindingInput.SELECT
                && now.output.equals(VirtualOutput.joystickButton(0))) {
                hasJoySelect = true;
                expected.add(InputBinding.digital(BindingInput.VOLUME_UP,
                    VirtualOutput.key(VirtualKey.ENTER)));
                expected.add(InputBinding.digital(BindingInput.VOLUME_DOWN,
                    VirtualOutput.key(VirtualKey.ESCAPE)));
            }
        }
        if (!hasJoySelect || expected.size() != saved.bindings.size())
            return false;
        for (int i = 0; i < expected.size(); i++) {
            InputBinding a = saved.bindings.get(i), b = expected.get(i);
            if (a.input != b.input || !a.output.equals(b.output)
                || a.transform != b.transform
                || a.deadzone != b.deadzone || a.threshold != b.threshold)
                return false;
            if (a.scale != b.scale && !(a.input == BindingInput.HEAD_YAW_DELTA
                && a.output.equals(VirtualOutput.mouseRelativeX())
                && a.scale == -b.scale)) return false;
        }
        return true;
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
        addDosMenuBindings(bindings, true);
    }

    private static void addDosMenuBindings(
        List<InputBinding> bindings, boolean rockerForMenu) {
        bindings.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.joystickButton(0)));
        if (rockerForMenu) {
            bindings.add(InputBinding.digital(BindingInput.VOLUME_UP,
                VirtualOutput.key(VirtualKey.ENTER)));
            bindings.add(InputBinding.digital(BindingInput.VOLUME_DOWN,
                VirtualOutput.key(VirtualKey.ESCAPE)));
        }
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

        // Keyboard-driven freeware FPS: arrows, Ctrl/fire, Space/use.
        // The physical O/Home remains in the VR shell, never guest mapped.
        List<InputBinding> ken = new ArrayList<>();
        addDosTouchpadArrows(ken);
        ken.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.key(VirtualKey.CTRL)));
        ken.add(InputBinding.digital(BindingInput.DAYDREAM_APP_BUTTON,
            VirtualOutput.key(VirtualKey.SPACE)));
        addDosMenuBindings(ken);
        profiles.add(new BindingProfile(ID_DOS_KENS_LABYRINTH,
            "DOS Ken's Labyrinth — move / fire / use", ken));

        // Jetpack: arrows move, ALT jump/thrust, CTRL phase.
        List<InputBinding> jetpack = new ArrayList<>();
        addDosTouchpadArrows(jetpack);
        jetpack.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.key(VirtualKey.ALT)));
        jetpack.add(InputBinding.digital(BindingInput.DAYDREAM_APP_BUTTON,
            VirtualOutput.key(VirtualKey.CTRL)));
        addDosMenuBindings(jetpack);
        profiles.add(new BindingProfile(ID_DOS_JETPACK,
            "DOS Jetpack — move / thrust / phase", jetpack));

        // Squarez one-player: arrows move, Ctrl rotate, Alt place.
        List<InputBinding> squarez = new ArrayList<>();
        addDosTouchpadArrows(squarez);
        squarez.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.key(VirtualKey.CTRL)));
        squarez.add(InputBinding.digital(BindingInput.DAYDREAM_APP_BUTTON,
            VirtualOutput.key(VirtualKey.ALT)));
        addDosMenuBindings(squarez);
        profiles.add(new BindingProfile(ID_DOS_SQUAREZ,
            "DOS Squarez — move / rotate / place", squarez));

        List<InputBinding> doom = new ArrayList<>();
        // Guest yaw from the same 3D head view is positive to the RIGHT.
        // The original Doom mouse turns horizontally; vertical head tilt
        // separately drives its Forward/Back keys.
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
        // Touchpad click = held mouse 1; physical App/minus = held mouse 2.
        // DOOM.CFG already configures mouse button 2 for strafing.
        doom.add(InputBinding.digital(BindingInput.SELECT,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT)));
        doom.add(InputBinding.digital(BindingInput.DAYDREAM_APP_BUTTON,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_RIGHT)));
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
        // Volume +/- selects Doom weapon slots, never Enter/Escape.
        addDosMenuBindings(doom, false);
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
        // Shared 3D head-view axes: right is positive X, up is negative Y
        // in the DOS guest mouse coordinate system.
        cursor.add(InputBinding.analog(BindingInput.HEAD_YAW_DELTA,
            VirtualOutput.mouseRelativeX(), 650.0f, 0.0f));
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
        // All headset-mouse modes use the SAME 3D-view-based axes.
        // Positive input yaw is right; positive pitch is up.
        headMouse.add(InputBinding.analog(BindingInput.HEAD_YAW_DELTA,
            VirtualOutput.mouseRelativeX(), 650.0f, 0.0f));
        headMouse.add(InputBinding.analog(BindingInput.HEAD_PITCH_DELTA,
            VirtualOutput.mouseRelativeY(), -650.0f, 0.0f));
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
