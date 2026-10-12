package io.github.mrcalzon02.reverievr;

import java.util.Locale;

/**
 * Selects the spatial surface and packed-eye interpretation independently of
 * the Android video decoder. Unknown media defaults to normal 2D playback.
 */
enum VideoProjection {
    FLAT_CINEMA("2D cinema", false, 0),
    FLAT_STEREO_SBS("3D cinema · side-by-side", false, 1),
    FLAT_STEREO_TB("3D cinema · top/bottom", false, 2),
    MONO_EQUIRECTANGULAR_360("2D 360°", true, 0),
    STEREO_EQUIRECTANGULAR_360_SBS("3D 360° · side-by-side", true, 1),
    STEREO_EQUIRECTANGULAR_360_TB("3D 360° · top/bottom", true, 2),
    MONO_EQUIRECTANGULAR_180("2D 180° equirect", true, 0),
    STEREO_EQUIRECTANGULAR_180_SBS("3D 180° equirect · SBS", true, 1),
    STEREO_EQUIRECTANGULAR_180_TB("3D 180° equirect · top/bottom", true, 2),
    HEADSET_RECORDING_MONO("Headset recording · mono", false, 0),
    HEADSET_RECORDING_SBS("Headset recording · side-by-side", false, 1),
    HEADSET_RECORDING_TB("Headset recording · top/bottom", false, 2);

    private final String label;
    private final boolean immersive;
    private final int packing; // 0 = mono, 1 = left/right, 2 = top/bottom

    VideoProjection(String label, boolean immersive, int packing) {
        this.label = label;
        this.immersive = immersive;
        this.packing = packing;
    }

    String displayName() {
        return label;
    }

    boolean isImmersive() {
        return immersive;
    }

    boolean isStereo() {
        return packing != 0;
    }

    /** Baked eye video is not a head-tracked spatial scene. */
    boolean isHeadsetRecording() {
        return this == HEADSET_RECORDING_MONO
            || this == HEADSET_RECORDING_SBS
            || this == HEADSET_RECORDING_TB;
    }

    boolean isEquirectangular180() {
        return this == MONO_EQUIRECTANGULAR_180
            || this == STEREO_EQUIRECTANGULAR_180_SBS
            || this == STEREO_EQUIRECTANGULAR_180_TB;
    }

    /**
     * 180-degree equirectangular pixels cover the front hemisphere only.
     * Camera-native VR180 fisheye requires calibrated per-eye mesh metadata;
     * do not reinterpret it using this projection.
     */
    float horizontalSpanRadians() {
        return isEquirectangular180()
            ? (float) Math.PI : (float) (Math.PI * 2.0);
    }

    VideoProjection next() {
        VideoProjection[] modes = values();
        return modes[(ordinal() + 1) % modes.length];
    }

    /**
     * Packed frames have two views; their combined decoder aspect is not the
     * visible aspect of either eye's image on a floating cinema surface.
     */
    float eyeDisplayAspect(float packedAspect) {
        if (packing == 1) {
            return packedAspect * 0.5f;
        }
        if (packing == 2) {
            return packedAspect * 2.0f;
        }
        return packedAspect;
    }

    /** Aspect-fit a recorded eye in the actual Cardboard eye viewport. */
    void fillHeadsetFitScale(float packedAspect, float viewportAspect,
                             float[] out) {
        out[0] = 1.0f;
        out[1] = 1.0f;
        if (!Float.isFinite(packedAspect) || packedAspect <= 0.0f
            || !Float.isFinite(viewportAspect) || viewportAspect <= 0.0f) {
            return;
        }
        float sourceAspect = eyeDisplayAspect(packedAspect);
        if (sourceAspect > viewportAspect) {
            out[1] = viewportAspect / sourceAspect;
        } else {
            out[0] = sourceAspect / viewportAspect;
        }
    }

    /**
     * Region in untransformed video UV coordinates: offsetX, offsetY, scaleX,
     * scaleY. Apply this BEFORE SurfaceTexture's producer transform.
     * Reuse the supplied buffer for both eyes; never allocate in drawEye.
     */
    void fillEyeUvRegion(boolean leftEye, float[] out) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 1.0f;
        out[3] = 1.0f;
        if (packing == 1) {
            out[0] = leftEye ? 0.0f : 0.5f;
            out[2] = 0.5f;
        } else if (packing == 2) {
            // In the onscreen UV convention, the top image is y=0.5..1.
            out[1] = leftEye ? 0.5f : 0.0f;
            out[3] = 0.5f;
        }
    }

    /**
     * Filename tags are only hints, not a claim to detect embedded stereo
     * metadata or infer 3D from a wide aspect ratio. Users can override in VR.
     */
    static VideoProjection suggestForFileName(String fileName) {
        if (fileName == null || fileName.trim().isEmpty()) {
            return FLAT_CINEMA;
        }
        String words = " " + fileName.toLowerCase(Locale.ROOT)
            .replaceAll("[^a-z0-9]+", " ").trim() + " ";
        // Treat 180 as equirectangular only with an explicit EQ tag.
        // Bare "VR180" may refer to a camera-specific dual-fisheye mesh.
        boolean hemisphere = words.contains(" eq180 ")
            || words.contains(" equirect180 ")
            || words.contains(" equirectangular180 ")
            || (words.contains(" 180 ")
                && (words.contains(" equirectangular ")
                    || words.contains(" equi ")
                    || words.contains(" eq ")));
        boolean panorama = words.contains(" 360 ")
            || words.contains(" equirectangular ")
            || words.contains(" eq360 ");
        boolean sbs = words.contains(" sbs ")
            || words.contains(" sidebyside ")
            || words.contains(" side by side ")
            || words.contains(" left right ");
        boolean tb = words.contains(" tb ")
            || words.contains(" topbottom ")
            || words.contains(" top bottom ")
            || words.contains(" overunder ")
            || words.contains(" over under ")
            || words.contains(" ou ");
        // A camera-native VR180 fisheye has calibrated per-eye projection
        // meshes. Its raw packed frames are not ordinary flat 3D cinema.
        // Explicit equirectangular 180 tags above remain supported.
        if (!hemisphere && (words.contains(" vr180 ")
            || words.contains(" fisheye180 ")
            || words.contains(" dualfisheye "))) {
            return FLAT_CINEMA;
        }
        // Only explicit *headset output* tags suggest fixed-eye playback.
        // SBS on its own can mean a regular stereo movie or spatial media.
        boolean headset = words.contains(" headset ")
            || words.contains(" hmd ")
            || words.contains(" headsetcapture ")
            || words.contains(" hmdcapture ")
            || words.contains(" vrmirror ");
        boolean recording = words.contains(" capture ")
            || words.contains(" recording ")
            || words.contains(" mirror ")
            || words.contains(" output ")
            || words.contains(" headsetcapture ")
            || words.contains(" hmdcapture ")
            || words.contains(" vrmirror ");
        if (headset && recording && !hemisphere && !panorama) {
            if (sbs && tb) return FLAT_CINEMA;
            return sbs ? HEADSET_RECORDING_SBS
                : tb ? HEADSET_RECORDING_TB : HEADSET_RECORDING_MONO;
        }
        // Conflicting tags are ambiguous: stay mono and require a user choice.
        if (sbs == tb) {
            return hemisphere ? MONO_EQUIRECTANGULAR_180
                : panorama ? MONO_EQUIRECTANGULAR_360 : FLAT_CINEMA;
        }
        if (hemisphere) {
            return sbs ? STEREO_EQUIRECTANGULAR_180_SBS
                : STEREO_EQUIRECTANGULAR_180_TB;
        }
        if (panorama) {
            return sbs
                ? STEREO_EQUIRECTANGULAR_360_SBS
                : STEREO_EQUIRECTANGULAR_360_TB;
        }
        return sbs ? FLAT_STEREO_SBS : FLAT_STEREO_TB;
    }

    static VideoProjection fromPreference(String value) {
        if (value == null || value.trim().isEmpty()) {
            return FLAT_CINEMA;
        }
        try {
            return valueOf(value);
        } catch (IllegalArgumentException exception) {
            return FLAT_CINEMA;
        }
    }
}
