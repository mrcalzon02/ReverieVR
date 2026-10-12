package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class DoomSharewareBootstrapPlanTest {
    @Test
    public void firstLaunchNeverRequiresInstallerKeyboardInput() {
        String yml = DoomSharewareBootstrapPlan.playYml();
        assertTrue(yml.contains("run_path: C:\\DOOMS\\DOOM.EXE"));
        assertFalse(yml.contains("run_input:"));
        assertFalse(yml.contains("DOSBOX.BAT"));
        assertFalse(yml.contains("DEICE"));
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
