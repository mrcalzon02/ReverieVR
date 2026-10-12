package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class HeadsetRecordingProjectionTest {
    @Test public void onlyExplicitCapturedHeadsetOutputSelectsFixedEyes() {
        assertEquals(VideoProjection.HEADSET_RECORDING_SBS,
            VideoProjection.suggestForFileName("headset_capture_sbs.mp4"));
        assertEquals(VideoProjection.HEADSET_RECORDING_TB,
            VideoProjection.suggestForFileName("hmd_mirror_top-bottom.mkv"));
        assertEquals(VideoProjection.HEADSET_RECORDING_MONO,
            VideoProjection.suggestForFileName("headset_recording.mp4"));
        assertEquals(VideoProjection.HEADSET_RECORDING_SBS,
            VideoProjection.suggestForFileName("VRmirror_SBS.mp4"));
        assertEquals(VideoProjection.FLAT_STEREO_SBS,
            VideoProjection.suggestForFileName("movie_SBS.mp4"));
        assertEquals(VideoProjection.FLAT_CINEMA,
            VideoProjection.suggestForFileName("headset_capture_sbs_tb.mp4"));
        assertEquals(VideoProjection.STEREO_EQUIRECTANGULAR_360_SBS,
            VideoProjection.suggestForFileName("headset_360_sbs_capture.mp4"));
        assertEquals(VideoProjection.FLAT_CINEMA,
            VideoProjection.suggestForFileName("VR180_SBS_clip.mp4"));
    }

    @Test public void fittedEyePreservesCapturedAspect() {
        float[] fit = new float[2];
        VideoProjection.HEADSET_RECORDING_SBS.fillHeadsetFitScale(
            32f / 9f, 1.0f, fit);
        assertEquals(1.0f, fit[0], 0.0001f);
        assertEquals(9f / 16f, fit[1], 0.0001f);
        VideoProjection.HEADSET_RECORDING_TB.fillHeadsetFitScale(
            8f / 9f, 2.0f, fit);
        assertEquals(8f / 9f, fit[0], 0.0001f);
        assertEquals(1.0f, fit[1], 0.0001f);
        VideoProjection.HEADSET_RECORDING_MONO.fillHeadsetFitScale(
            1.0f, 1.0f, fit);
        assertEquals(1.0f, fit[0], 0.0001f);
        assertEquals(1.0f, fit[1], 0.0001f);
        VideoProjection.HEADSET_RECORDING_SBS.fillHeadsetFitScale(
            Float.NaN, 1.0f, fit);
        assertEquals(1.0f, fit[0], 0.0001f);
        assertEquals(1.0f, fit[1], 0.0001f);
    }

    @Test public void playbackCategoryIsNeitherCinemaNorSpatial() {
        assertTrue(VideoProjection.HEADSET_RECORDING_MONO.isHeadsetRecording());
        assertFalse(VideoProjection.HEADSET_RECORDING_MONO.isStereo());
        assertTrue(VideoProjection.HEADSET_RECORDING_TB.isStereo());
        assertFalse(VideoProjection.HEADSET_RECORDING_TB.isImmersive());
    }
}
