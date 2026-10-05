package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

final class BindingProfile {
    final String id;
    final String displayName;
    final List<InputBinding> bindings;

    BindingProfile(
        String id,
        String displayName,
        List<InputBinding> bindings
    ) {
        String safeId = id == null ? "" : id.trim();
        String safeName = displayName == null ? "" : displayName.trim();

        if (safeId.isEmpty()) {
            throw new IllegalArgumentException("Binding profile id is required.");
        }
        if (safeName.isEmpty()) {
            throw new IllegalArgumentException("Binding profile name is required.");
        }

        this.id = safeId;
        this.displayName = safeName;
        this.bindings = Collections.unmodifiableList(
            new ArrayList<>(
                Objects.requireNonNull(bindings, "bindings")
            )
        );
    }

    BindingProfile withBindings(List<InputBinding> replacement) {
        return new BindingProfile(id, displayName, replacement);
    }
}
