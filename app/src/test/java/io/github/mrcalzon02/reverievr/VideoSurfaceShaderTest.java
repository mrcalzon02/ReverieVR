package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.lang.reflect.Field;
import java.util.regex.Pattern;
import org.junit.Test;

public final class VideoSurfaceShaderTest {
    @Test public void fragmentShaderAvoidsReservedPackedIdentifier() throws Exception {
        Field field = VideoSurfaceRenderer.class.getDeclaredField("FRAGMENT_SHADER");
        field.setAccessible(true);
        String fragmentShader = (String) field.get(null);

        // GLSL ES 1.00 reserves "packed" even on devices that do not use it.
        // This must validate the actual shader sent to glShaderSource, not a
        // duplicated fixture that could diverge from production.
        assertFalse("GLSL reserved identifier 'packed' in video fragment shader",
            Pattern.compile("\\bpacked\\b").matcher(fragmentShader).find());

        // Retain the source-eye crop and SurfaceTexture transform after rename.
        assertTrue(fragmentShader.contains(
            "vec2 packedUv = u_EyeRegion.xy + local * u_EyeRegion.zw;"));
        assertTrue(fragmentShader.contains(
            "vec2 tex = (u_SurfaceTransform * vec4(packedUv, 0.0, 1.0)).xy;"));
        assertTrue(fragmentShader.contains(
            "gl_FragColor = texture2D(u_Texture, tex);"));
    }
}
