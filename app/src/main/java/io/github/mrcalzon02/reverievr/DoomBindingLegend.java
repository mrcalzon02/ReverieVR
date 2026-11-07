package io.github.mrcalzon02.reverievr;

/**
 * The binocular Doom legend reflects the LIVE selected input profile,
 * not an unrelated hardcoded list of aspirational controller actions.
 * An unsupported or customized binding is labelled, never fabricated.
 */
final class DoomBindingLegend {
    private DoomBindingLegend() {}

    static String[] lines(BindingProfile profile) {
        if (profile == null) {
            return new String[] {"NO ACTIVE INPUT PROFILE"};
        }
        boolean yaw = has(profile, BindingInput.HEAD_YAW_DELTA,
            VirtualOutput.mouseRelativeX());
        boolean forward = has(profile, BindingInput.TOUCHPAD_Y,
            VirtualOutput.key(VirtualKey.UP));
        boolean back = has(profile, BindingInput.TOUCHPAD_Y,
            VirtualOutput.key(VirtualKey.DOWN));
        boolean side = has(profile, BindingInput.TOUCHPAD_X,
            VirtualOutput.key(VirtualKey.COMMA))
            && has(profile, BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.PERIOD));
        boolean turn = has(profile, BindingInput.TOUCHPAD_X,
            VirtualOutput.key(VirtualKey.LEFT))
            && has(profile, BindingInput.TOUCHPAD_X,
                VirtualOutput.key(VirtualKey.RIGHT));
        boolean fire = has(profile, BindingInput.SELECT,
            VirtualOutput.key(VirtualKey.CTRL))
            || has(profile, BindingInput.SELECT,
                VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT));
        boolean use = has(profile, BindingInput.DAYDREAM_APP_BUTTON,
            VirtualOutput.key(VirtualKey.SPACE));
        boolean strafeApp = has(profile, BindingInput.DAYDREAM_APP_BUTTON,
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_RIGHT));
        boolean weapons = BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE.equals(profile.id)
            && !hasAnyDigital(profile, BindingInput.VOLUME_UP)
            && !hasAnyDigital(profile, BindingInput.VOLUME_DOWN);
        return new String[] {
            yaw ? "HEAD TURN : MOUSE AIM" : "HEAD TURN : NOT BOUND",
            forward && back ? "PAD UP/DN : WALK" : "PAD UP/DN : CUSTOM / OFF",
            side ? "PAD L/R  : STRAFE"
                : turn ? "PAD L/R  : TURN" : "PAD L/R  : CUSTOM / OFF",
            fire ? "PAD CLICK: FIRE" : "PAD CLICK: CUSTOM / OFF",
            use ? "APP / -  : USE / OPEN"
                : strafeApp ? "APP / -  : STRAFE" : "APP / -  : CUSTOM / OFF",
            weapons ? "VOL +/-  : WEAPON +/-" : "VOL +/-  : CUSTOM / OFF",
            "HOME / O : SHELL MENU*",
            "* SYSTEM BUTTONS MAY BE RESERVED"
        };
    }

    private static boolean has(BindingProfile p, BindingInput input,
                               VirtualOutput output) {
        for (InputBinding b : p.bindings) {
            if (b.input == input && b.output.equals(output)) return true;
        }
        return false;
    }

    private static boolean hasAnyDigital(BindingProfile p, BindingInput input) {
        for (InputBinding b : p.bindings) {
            if (b.input == input && b.transform == BindingTransform.DIGITAL)
                return true;
        }
        return false;
    }
}
