package io.github.mrcalzon02.reverievr;

import java.util.Objects;

final class InputBinding {
    final BindingInput input;
    final VirtualOutput output;
    final BindingTransform transform;
    final float scale;
    final float deadzone;
    final float threshold;

    InputBinding(
        BindingInput input,
        VirtualOutput output,
        BindingTransform transform,
        float scale,
        float deadzone,
        float threshold
    ) {
        this.input = Objects.requireNonNull(input, "input");
        this.output = Objects.requireNonNull(output, "output");
        this.transform = Objects.requireNonNull(transform, "transform");
        this.scale = scale;
        this.deadzone = clamp(Math.abs(deadzone), 0.0f, 0.95f);
        this.threshold = clamp(Math.abs(threshold), 0.05f, 1.0f);
    }

    static InputBinding digital(
        BindingInput input,
        VirtualOutput output
    ) {
        return new InputBinding(
            input,
            output,
            BindingTransform.DIGITAL,
            1.0f,
            0.0f,
            0.5f
        );
    }

    static InputBinding analog(
        BindingInput input,
        VirtualOutput output,
        float scale,
        float deadzone
    ) {
        return new InputBinding(
            input,
            output,
            BindingTransform.ANALOG,
            scale,
            deadzone,
            0.5f
        );
    }

    static InputBinding positiveThreshold(
        BindingInput input,
        VirtualOutput output,
        float threshold
    ) {
        return new InputBinding(
            input,
            output,
            BindingTransform.POSITIVE_THRESHOLD,
            1.0f,
            0.0f,
            threshold
        );
    }

    static InputBinding negativeThreshold(
        BindingInput input,
        VirtualOutput output,
        float threshold
    ) {
        return new InputBinding(
            input,
            output,
            BindingTransform.NEGATIVE_THRESHOLD,
            1.0f,
            0.0f,
            threshold
        );
    }

    private static float clamp(float value, float minimum, float maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }
}
