package io.github.mrcalzon02.reverievr;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.Locale;

final class BindingProfileCodec {
    private static final String VERSION = "RVBIND1";

    private BindingProfileCodec() {
    }

    static String encode(BindingProfile profile) {
        if (profile == null) {
            throw new IllegalArgumentException("Binding profile is required.");
        }

        StringBuilder result = new StringBuilder();
        result.append(VERSION)
            .append('\t')
            .append(text(profile.id))
            .append('\t')
            .append(text(profile.displayName))
            .append('\n');

        for (InputBinding binding : profile.bindings) {
            result.append("B")
                .append('\t').append(binding.input.name())
                .append('\t').append(binding.output.kind.name())
                .append('\t').append(binding.output.code)
                .append('\t').append(binding.transform.name())
                .append('\t').append(Float.toString(binding.scale))
                .append('\t').append(Float.toString(binding.deadzone))
                .append('\t').append(Float.toString(binding.threshold))
                .append('\n');
        }

        return result.toString();
    }

    static BindingProfile decode(String encoded) {
        if (encoded == null || encoded.trim().isEmpty()) {
            throw new IllegalArgumentException("Binding profile data is empty.");
        }

        String[] lines = encoded.split("\\n");
        if (lines.length == 0) {
            throw new IllegalArgumentException("Binding profile header is missing.");
        }

        String[] header = lines[0].split("\\t", -1);
        if (header.length != 3 || !VERSION.equals(header[0])) {
            throw new IllegalArgumentException("Unsupported binding profile version.");
        }

        String id = fromText(header[1]);
        String name = fromText(header[2]);
        List<InputBinding> bindings = new ArrayList<>();

        for (int index = 1; index < lines.length; index++) {
            String line = lines[index];
            if (line.trim().isEmpty()) {
                continue;
            }

            String[] fields = line.split("\\t", -1);
            if (fields.length != 8 || !"B".equals(fields[0])) {
                throw new IllegalArgumentException(
                    "Malformed binding entry at line " + (index + 1)
                );
            }

            BindingInput input = BindingInput.valueOf(fields[1]);
            VirtualOutputKind kind = VirtualOutputKind.valueOf(fields[2]);
            int code = Integer.parseInt(fields[3]);
            BindingTransform transform =
                BindingTransform.valueOf(fields[4]);
            float scale = Float.parseFloat(fields[5]);
            float deadzone = Float.parseFloat(fields[6]);
            float threshold = Float.parseFloat(fields[7]);

            bindings.add(
                new InputBinding(
                    input,
                    output(kind, code),
                    transform,
                    scale,
                    deadzone,
                    threshold
                )
            );
        }

        return new BindingProfile(id, name, bindings);
    }

    private static VirtualOutput output(
        VirtualOutputKind kind,
        int code
    ) {
        switch (kind) {
            case KEY:
                if (code < 0 || code >= VirtualKey.values().length) {
                    throw new IllegalArgumentException("Invalid virtual key code.");
                }
                return VirtualOutput.key(VirtualKey.values()[code]);

            case MOUSE_BUTTON:
                return VirtualOutput.mouseButton(code);

            case MOUSE_REL_X:
                return VirtualOutput.mouseRelativeX();

            case MOUSE_REL_Y:
                return VirtualOutput.mouseRelativeY();

            case MOUSE_ABS_X:
                return VirtualOutput.mouseAbsoluteX();

            case MOUSE_ABS_Y:
                return VirtualOutput.mouseAbsoluteY();

            case JOYSTICK_AXIS_X:
                return VirtualOutput.joystickAxisX();

            case JOYSTICK_AXIS_Y:
                return VirtualOutput.joystickAxisY();

            case JOYSTICK_BUTTON:
                return VirtualOutput.joystickButton(code);

            default:
                throw new IllegalArgumentException(
                    String.format(Locale.US, "Unsupported output kind: %s", kind)
                );
        }
    }

    private static String text(String value) {
        return Base64.getUrlEncoder()
            .withoutPadding()
            .encodeToString(
                value.getBytes(StandardCharsets.UTF_8)
            );
    }

    private static String fromText(String value) {
        return new String(
            Base64.getUrlDecoder().decode(value),
            StandardCharsets.UTF_8
        );
    }
}
