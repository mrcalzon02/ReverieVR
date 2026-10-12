package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class VideoProjectionTest {
    @Test public void ordinaryVideoNeverAssumesStereoFromItsAspect() {
        assertEquals(VideoProjection.FLAT_CINEMA,
            VideoProjection.suggestForFileName("wide_3840x1080.mp4"));
        assertEquals(VideoProjection.FLAT_CINEMA,
            VideoProjection.suggestForFileName("3d_sample.mp4"));
        // Camera-native VR180 normally embeds calibrated mesh projection.
        // It is not equivalent to a generic 180 equirectangular panorama.
        assertEquals(VideoProjection.FLAT_CINEMA,
            VideoProjection.suggestForFileName("VR180_SBS_clip.mp4"));
        assertEquals(VideoProjection.FLAT_CINEMA,
            VideoProjection.fromPreference("unrecognized-format"));
        assertEquals(VideoProjection.FLAT_CINEMA,
            VideoProjection.fromPreference(null));
    }

    @Test public void specificFilenameHintsSelectPackedEyeModes() {
        assertEquals(VideoProjection.FLAT_STEREO_SBS,
            VideoProjection.suggestForFileName("Movie.3D.SBS.mkv"));
        assertEquals(VideoProjection.FLAT_STEREO_TB,
            VideoProjection.suggestForFileName("concert_top-bottom.mp4"));
        assertEquals(VideoProjection.MONO_EQUIRECTANGULAR_360,
            VideoProjection.suggestForFileName("tour_360.mp4"));
        assertEquals(VideoProjection.STEREO_EQUIRECTANGULAR_360_SBS,
            VideoProjection.suggestForFileName("trip_360_sbs.mp4"));
        assertEquals(VideoProjection.STEREO_EQUIRECTANGULAR_360_TB,
            VideoProjection.suggestForFileName("underwater_360_ou.mp4"));
        assertEquals(VideoProjection.MONO_EQUIRECTANGULAR_360,
            VideoProjection.suggestForFileName("clip_360_sbs_tb.mp4"));
        assertEquals(VideoProjection.STEREO_EQUIRECTANGULAR_180_SBS,
            VideoProjection.suggestForFileName("beach_180_equirectangular_sbs.mp4"));
        assertEquals(VideoProjection.STEREO_EQUIRECTANGULAR_180_TB,
            VideoProjection.suggestForFileName("reef_eq180_top-bottom.mp4"));
        assertEquals(VideoProjection.MONO_EQUIRECTANGULAR_180,
            VideoProjection.suggestForFileName("view_equirect180.mp4"));
    }

    @Test public void eachEyeGetsOnlyItsOwnHalfOfPackedVideo() {
        float[] crop = new float[4];
        VideoProjection.FLAT_STEREO_SBS.fillEyeUvRegion(true, crop);
        assertRegion(crop, 0, 0, 0.5f, 1);
        VideoProjection.FLAT_STEREO_SBS.fillEyeUvRegion(false, crop);
        assertRegion(crop, 0.5f, 0, 0.5f, 1);
        VideoProjection.STEREO_EQUIRECTANGULAR_360_TB
            .fillEyeUvRegion(true, crop);
        assertRegion(crop, 0, 0.5f, 1, 0.5f);
        VideoProjection.STEREO_EQUIRECTANGULAR_360_TB
            .fillEyeUvRegion(false, crop);
        assertRegion(crop, 0, 0, 1, 0.5f);
        VideoProjection.FLAT_CINEMA.fillEyeUvRegion(false, crop);
        assertRegion(crop, 0, 0, 1, 1);
    }

    @Test public void packedAspectIsCorrectedPerEye() {
        assertEquals(16f / 9f,
            VideoProjection.FLAT_STEREO_SBS.eyeDisplayAspect(32f / 9f),
            0.0001f);
        assertEquals(16f / 9f,
            VideoProjection.FLAT_STEREO_TB.eyeDisplayAspect(8f / 9f),
            0.0001f);
        assertEquals(16f / 9f,
            VideoProjection.FLAT_CINEMA.eyeDisplayAspect(16f / 9f),
            0.0001f);
    }

    @Test public void hemisphereCoversFrontHalfNotEntireSphere() {
        assertEquals((float) Math.PI,
            VideoProjection.MONO_EQUIRECTANGULAR_180.horizontalSpanRadians(),
            0.0001f);
        assertEquals((float) (2 * Math.PI),
            VideoProjection.MONO_EQUIRECTANGULAR_360.horizontalSpanRadians(),
            0.0001f);
        assertTrue(VideoProjection.STEREO_EQUIRECTANGULAR_180_TB
            .isEquirectangular180());
        assertFalse(VideoProjection.FLAT_STEREO_SBS.isEquirectangular180());
    }

    @Test public void stereo180UsesExistingPackedEyeCrop() {
        float[] region = new float[4];
        VideoProjection.STEREO_EQUIRECTANGULAR_180_SBS
            .fillEyeUvRegion(true, region);
        assertRegion(region, 0, 0, 0.5f, 1);
        VideoProjection.STEREO_EQUIRECTANGULAR_180_SBS
            .fillEyeUvRegion(false, region);
        assertRegion(region, 0.5f, 0, 0.5f, 1);
    }

    @Test public void fixedEyeRecordingUsesDistinctStereoRegions() {
        assertTrue(VideoProjection.HEADSET_RECORDING_SBS.isHeadsetRecording());
        assertFalse(VideoProjection.FLAT_STEREO_SBS.isHeadsetRecording());
        assertFalse(VideoProjection.HEADSET_RECORDING_MONO.isImmersive());
        float[] crop = new float[4];
        VideoProjection.HEADSET_RECORDING_SBS.fillEyeUvRegion(true, crop);
        assertRegion(crop, 0, 0, 0.5f, 1);
        VideoProjection.HEADSET_RECORDING_SBS.fillEyeUvRegion(false, crop);
        assertRegion(crop, 0.5f, 0, 0.5f, 1);
        VideoProjection.HEADSET_RECORDING_TB.fillEyeUvRegion(true, crop);
        assertRegion(crop, 0, 0.5f, 1, 0.5f);
    }

    @Test public void cyclingKeepsMonoAndStereoDistinct() {
        assertFalse(VideoProjection.FLAT_CINEMA.isStereo());
        assertTrue(VideoProjection.FLAT_STEREO_SBS.isStereo());
        assertTrue(VideoProjection.STEREO_EQUIRECTANGULAR_360_SBS.isImmersive());
        VideoProjection mode = VideoProjection.FLAT_CINEMA;
        for (int i = 0; i < VideoProjection.values().length; i++) {
            mode = mode.next();
        }
        assertEquals(VideoProjection.FLAT_CINEMA, mode);
    }

    private static void assertRegion(float[] region,
                                     float x, float y, float w, float h) {
        assertEquals(x, region[0], 0.0001f);
        assertEquals(y, region[1], 0.0001f);
        assertEquals(w, region[2], 0.0001f);
        assertEquals(h, region[3], 0.0001f);
    }
}
