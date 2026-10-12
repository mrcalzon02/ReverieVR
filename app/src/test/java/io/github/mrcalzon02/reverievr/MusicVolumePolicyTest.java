package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class MusicVolumePolicyTest {
    @Test public void preservesLegacyDefaultAndDoesNotAffectSystemVolume() {
        assertEquals(0.22f,
            MusicVolumePolicy.gain(MusicVolumePolicy.DEFAULT_PERCENT, false),
            0.000001f);
        assertEquals(0f, MusicVolumePolicy.gain(100, true), 0f);
        assertEquals(0.50f, MusicVolumePolicy.gain(100, false), 0.000001f);
    }

    @Test public void clampsInvalidStoredAndEdgeValues() {
        assertEquals(0, MusicVolumePolicy.clamp(-33));
        assertEquals(100, MusicVolumePolicy.clamp(500));
        assertEquals(0f, MusicVolumePolicy.gain(-2, false), 0f);
        assertEquals(0.50f, MusicVolumePolicy.gain(200, false), 0.000001f);
        assertEquals(100, MusicVolumePolicy.adjust(95, 1));
        assertEquals(0, MusicVolumePolicy.adjust(5, -1));
        assertEquals(0, MusicVolumePolicy.adjust(-50, 0));
        assertEquals(100, MusicVolumePolicy.adjust(500, 0));
    }

    @Test public void volumeAndMuteRemainIndependent() {
        assertEquals(54, MusicVolumePolicy.adjust(44, 1));
        assertEquals(34, MusicVolumePolicy.adjust(44, -1));
        assertEquals("MUSIC 44% MUTED", MusicVolumePolicy.display(44, true));
        assertEquals("MUSIC 44% ON", MusicVolumePolicy.display(44, false));
        assertEquals(0.22f, MusicVolumePolicy.gain(44, false), 0.000001f);
    }
}
