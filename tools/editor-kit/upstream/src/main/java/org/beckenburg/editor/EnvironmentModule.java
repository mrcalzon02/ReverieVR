package org.beckenburg.editor;

import java.util.List;
import java.util.Objects;

/**
 * Optional runtime/editor integration contract for water, weather, lighting and future
 * environment modules. The module owns its simulation and rendering; editor callers
 * may inspect capabilities without assuming the module is installed.
 */
public interface EnvironmentModule {
    enum Capability {
        LIVE_PREVIEW, AUTHORING, SERIALIZATION, PLAYTEST, DIAGNOSTICS
    }
    String id();
    String displayName();
    List<Capability> capabilities();

    /** A single module may attach to a scene once per editor session. */
    default void attach(RuntimeScene scene){Objects.requireNonNull(scene);}
    default void detach(){}

    /** True only when the module's real runtime integration is operational. */
    default boolean ready(){return false;}
}
