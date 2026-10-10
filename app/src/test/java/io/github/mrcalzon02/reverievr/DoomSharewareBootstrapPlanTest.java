package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class DoomSharewareBootstrapPlanTest {
    @Test
    public void installMetadataRunsBootstrapAndDeiceInputs() {
        String yml =
            DoomSharewareBootstrapPlan.installYml();

        assertTrue(yml.contains("run_path: DOSBOX.BAT"));
        assertTrue(yml.contains("(wait:1500)c"));
        assertTrue(yml.contains("(wait:900)(enter)"));
        assertFalse(yml.contains("c(enter)"));
        assertFalse(yml.contains("y(enter)"));
    }

    @Test
    public void installedMetadataBypassesInstallerInputs() {
        String yml =
            DoomSharewareBootstrapPlan.playYml();

        assertTrue(
            yml.contains(
                "run_path: C:\\DOOMS\\DOOM.EXE"
            )
        );
        assertFalse(yml.contains("run_input:"));
        assertFalse(yml.contains("DOSBOX.BAT"));
    }

    @Test
    public void bootstrapChainsOriginalInstallerToDoom() {
        String batch =
            DoomSharewareBootstrapPlan.bootstrapBatch();

        assertTrue(batch.contains("DEICE.EXE"));
        assertTrue(
            batch.contains("DOOMS_19.EXE -d")
        );
        assertTrue(
            batch.contains(
                "copy REVERIE.CFG DOOMS\\DEFAULT.CFG"
            )
        );
        assertTrue(batch.contains("DOOM.EXE"));
    }

    @Test
    public void bootstrapConfigMatchesDosboxSb16Baseline() {
        String config =
            DoomSharewareBootstrapPlan.doomConfig();

        assertTrue(config.contains("use_mouse 1"));
        assertTrue(config.contains("snd_musicdevice 0"));
        assertTrue(config.contains("snd_sfxdevice 3"));
        assertTrue(config.contains("snd_sbport 544"));
        assertTrue(config.contains("snd_sbirq 7"));
        assertTrue(config.contains("snd_sbdma 1"));
    }
}
