package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

final class BindingProfileTuner {
    private static final float MIN_ANALOG_SCALE = 0.05f;
    private static final float MAX_ANALOG_SCALE = 5000.0f;

    private BindingProfileTuner() {
    }

    static BindingProfile adjustAnalogScale(
        BindingProfile profile,
        float factor
    ) {
        if (profile == null) {
            throw new IllegalArgumentException(
                "Binding profile is required."
            );
        }
        if (!Float.isFinite(factor) || factor <= 0.0f) {
            throw new IllegalArgumentException(
                "Analog scale factor must be positive."
            );
        }

        List<InputBinding> replacement =
            new ArrayList<>(profile.bindings.size());
        boolean changed = false;

        for (InputBinding binding : profile.bindings) {
            if (binding.transform != BindingTransform.ANALOG) {
                replacement.add(binding);
                continue;
            }

            float scale = adjustScale(binding.scale, factor);
            replacement.add(
                new InputBinding(
                    binding.input,
                    binding.output,
                    binding.transform,
                    scale,
                    binding.deadzone,
                    binding.threshold
                )
            );
            changed = changed || scale != binding.scale;
        }

        return changed
            ? profile.withBindings(replacement)
            : profile;
    }

    static BindingProfile adjustAnalogDeadzone(
        BindingProfile profile,
        float delta
    ) {
        if (profile == null) {
            throw new IllegalArgumentException(
                "Binding profile is required."
            );
        }
        if (!Float.isFinite(delta)) {
            throw new IllegalArgumentException(
                "Analog deadzone delta must be finite."
            );
        }

        List<InputBinding> replacement =
            new ArrayList<>(profile.bindings.size());
        boolean changed = false;

        for (InputBinding binding : profile.bindings) {
            if (binding.transform != BindingTransform.ANALOG) {
                replacement.add(binding);
                continue;
            }

            float deadzone = clamp(
                binding.deadzone + delta,
                0.0f,
                0.95f
            );
            replacement.add(
                new InputBinding(
                    binding.input,
                    binding.output,
                    binding.transform,
                    binding.scale,
                    deadzone,
                    binding.threshold
                )
            );
            changed = changed || deadzone != binding.deadzone;
        }

        return changed
            ? profile.withBindings(replacement)
            : profile;
    }

    static int analogBindingCount(BindingProfile profile) {
        if (profile == null) {
            return 0;
        }

        int count = 0;
        for (InputBinding binding : profile.bindings) {
            if (binding.transform == BindingTransform.ANALOG) {
                count++;
            }
        }
        return count;
    }

    static String describeAnalog(BindingProfile profile) {
        if (profile == null) {
            return "No active profile";
        }

        int count = 0;
        float firstMagnitude = 0.0f;
        float firstDeadzone = 0.0f;
        boolean mixed = false;

        for (InputBinding binding : profile.bindings) {
            if (binding.transform != BindingTransform.ANALOG) {
                continue;
            }

            float magnitude = Math.abs(binding.scale);
            if (count == 0) {
                firstMagnitude = magnitude;
                firstDeadzone = binding.deadzone;
            } else if (Math.abs(firstMagnitude - magnitude) > 0.0001f
                || Math.abs(firstDeadzone - binding.deadzone) > 0.0001f) {
                mixed = true;
            }
            count++;
        }

        if (count == 0) {
            return "No analog bindings";
        }
        if (mixed) {
            return String.format(
                Locale.US,
                "%d analog bindings • mixed tuning",
                count
            );
        }

        return String.format(
            Locale.US,
            "%d analog • sensitivity %s • deadzone %.2f",
            count,
            formatScale(firstMagnitude),
            firstDeadzone
        );
    }

    private static float adjustScale(
        float scale,
        float factor
    ) {
        float sign = scale < 0.0f ? -1.0f : 1.0f;
        float magnitude = Math.abs(scale);
        if (!Float.isFinite(magnitude) || magnitude == 0.0f) {
            magnitude = 1.0f;
        }

        magnitude = clamp(
            magnitude * factor,
            MIN_ANALOG_SCALE,
            MAX_ANALOG_SCALE
        );
        return sign * magnitude;
    }

    private static String formatScale(float value) {
        if (value >= 100.0f) {
            return String.format(Locale.US, "%.0f", value);
        }
        if (value >= 10.0f) {
            return String.format(Locale.US, "%.1f", value);
        }
        return String.format(Locale.US, "%.2f", value);
    }

    private static float clamp(
        float value,
        float minimum,
        float maximum
    ) {
        return Math.max(minimum, Math.min(maximum, value));
    }
}
