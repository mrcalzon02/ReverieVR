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
    }

    synchronized void submitDigital(
        BindingInput input,
        boolean down
    ) {
        if (input == null) {
            return;
        }

        List<InputBinding> bindings = profile.bindings;
        for (int index = 0; index < bindings.size(); index++) {
            InputBinding binding = bindings.get(index);
            if (binding.input != input
                || binding.transform != BindingTransform.DIGITAL) {
                continue;
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
                    bus.applyAnalog(binding.output, analog);
                    break;

                case POSITIVE_THRESHOLD:
                    updateThreshold(
                        index,
                        binding.output,
                        safeValue >= binding.threshold
                    );
                    break;

                case NEGATIVE_THRESHOLD:
                    updateThreshold(
                        index,
                        binding.output,
                        safeValue <= -binding.threshold
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

    private void updateThreshold(
        int index,
        VirtualOutput output,
        boolean active
    ) {
        if (thresholdStates[index] == active) {
            return;
        }

        thresholdStates[index] = active;
        bus.applyDigital(output, active);
    }

    private static float clampAxis(float value) {
        return Math.max(-1.0f, Math.min(1.0f, value));
    }
}
