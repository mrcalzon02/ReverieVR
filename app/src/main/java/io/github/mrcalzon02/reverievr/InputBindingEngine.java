package io.github.mrcalzon02.reverievr;

import java.util.List;

final class InputBindingEngine {
    private final VirtualInputBus bus;

    private BindingProfile profile;
    private boolean[] thresholdStates = new boolean[0];

    InputBindingEngine(
        VirtualInputBus bus,
        BindingProfile profile
    ) {
        this.bus = bus;
        setProfile(profile);
    }

    synchronized BindingProfile getProfile() {
        return profile;
    }

    synchronized void setProfile(BindingProfile profile) {
        if (profile == null) {
            throw new IllegalArgumentException("Binding profile is required.");
        }

        if (this.profile != null) {
            releaseAll();
        }

        this.profile = profile;
        thresholdStates = new boolean[profile.bindings.size()];

        ReverieLog.milestone(
            "BINDING",
            "Active binding profile="
                + profile.id
                + " name="
                + profile.displayName
                + " bindings="
                + profile.bindings.size()
        );
    }

    synchronized void submitDigital(
        BindingInput input,
        boolean down
    ) {
        if (input == null) {
            return;
        }

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "BINDING_INPUT",
                "digital input="
                    + input
                    + " down="
                    + down
            );
        }

        List<InputBinding> bindings = profile.bindings;
        for (int index = 0; index < bindings.size(); index++) {
            InputBinding binding = bindings.get(index);
            if (binding.input != input
                || binding.transform != BindingTransform.DIGITAL) {
                continue;
            }

            if (ReverieLog.isDevelopment()) {
                ReverieLog.dev(
                    "BINDING_OUTPUT",
                    "digital "
                        + input
                        + " -> "
                        + binding.output
                        + " down="
                        + down
                );
            }

            bus.applyDigital(binding.output, down);
        }
    }

    synchronized void submitAxis(
        BindingInput input,
        float value
    ) {
        if (input == null) {
            return;
        }

        float safeValue = clampAxis(value);

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "BINDING_INPUT",
                "axis input="
                    + input
                    + " raw="
                    + value
                    + " clamped="
                    + safeValue
            );
        }
        List<InputBinding> bindings = profile.bindings;

        for (int index = 0; index < bindings.size(); index++) {
            InputBinding binding = bindings.get(index);
            if (binding.input != input) {
                continue;
            }

            switch (binding.transform) {
                case ANALOG:
                    float analog =
                        Math.abs(safeValue) < binding.deadzone
                            ? 0.0f
                            : safeValue * binding.scale;
                    if (ReverieLog.isDevelopment()) {
                        ReverieLog.dev(
                            "BINDING_OUTPUT",
                            "analog "
                                + input
                                + " -> "
                                + binding.output
                                + " value="
                                + analog
                                + " scale="
                                + binding.scale
                                + " deadzone="
                                + binding.deadzone
                        );
                    }
                    bus.applyAnalog(binding.output, analog);
                    break;

                case POSITIVE_THRESHOLD:
                    updateThreshold(
                        index,
                        binding.output,
                        safeValue >= thresholdFor(binding, index)
                    );
                    break;

                case NEGATIVE_THRESHOLD:
                    updateThreshold(
                        index,
                        binding.output,
                        safeValue <= -thresholdFor(binding, index)
                    );
                    break;

                case DIGITAL:
                default:
                    break;
            }
        }
    }

    synchronized void submitRelative(
        BindingInput input,
        float delta
    ) {
        if (input == null || delta == 0.0f) {
            return;
        }

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "BINDING_INPUT",
                "relative input="
                    + input
                    + " delta="
                    + delta
            );
        }

        List<InputBinding> bindings = profile.bindings;
        for (int index = 0; index < bindings.size(); index++) {
            InputBinding binding = bindings.get(index);
            if (binding.input != input
                || binding.transform != BindingTransform.ANALOG) {
                continue;
            }

            float value =
                Math.abs(delta) < binding.deadzone
                    ? 0.0f
                    : delta * binding.scale;
            if (value != 0.0f) {
                if (ReverieLog.isDevelopment()) {
                    ReverieLog.dev(
                        "BINDING_OUTPUT",
                        "relative "
                            + input
                            + " -> "
                            + binding.output
                            + " value="
                            + value
                    );
                }
                bus.applyAnalog(binding.output, value);
            }
        }
    }

    synchronized void pulse(BindingInput input) {
        submitDigital(input, true);
        submitDigital(input, false);
    }

    synchronized void releaseAll() {
        for (int index = 0; index < thresholdStates.length; index++) {
            thresholdStates[index] = false;
        }
        bus.releaseAll();
    }

    private float thresholdFor(InputBinding binding, int index) {
        // Head tilt uses a directional dead zone with hysteresis: enter
        // around +/-12 degrees, release around +/-8.5 degrees. Only the
        // absolute head-pitch input receives this new treatment.
        return binding.input == BindingInput.HEAD_PITCH_UP_AXIS
            && thresholdStates[index]
            ? binding.threshold * 0.70f : binding.threshold;
    }

    private void updateThreshold(
        int index,
        VirtualOutput output,
        boolean active
    ) {
        if (thresholdStates[index] == active) {
            return;
        }

        thresholdStates[index] = active;

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "BINDING_OUTPUT",
                "threshold index="
                    + index
                    + " output="
                    + output
                    + " active="
                    + active
            );
        }

        // Head pitch and the Daydream touchpad can both hold the same Doom
        // Up/Down key. Releasing one source must not cancel the other.
        boolean stillActive = false;
        for (int other = 0; other < thresholdStates.length; other++) {
            if (thresholdStates[other]
                && profile.bindings.get(other).output.equals(output)) {
                stillActive = true;
                break;
            }
        }
        bus.applyDigital(output, stillActive);
    }

    private static float clampAxis(float value) {
        return Math.max(-1.0f, Math.min(1.0f, value));
    }
}
