package io.github.mrcalzon02.reverievr;

final class DoomSharewareBootstrapPlan {
    private DoomSharewareBootstrapPlan() {
    }

    static String playYml() {
        return "run_path: C:\\DOOMS\\DOOM.EXE\n";
    }

    static String doomConfig() {
        return "mouse_sensitivity 5\r\n"
            + "sfx_volume 8\r\n"
            + "music_volume 0\r\n"
            + "show_messages 1\r\n"
            + "use_mouse 1\r\n"
            + "mouseb_fire 0\r\n"
            + "mouseb_strafe 1\r\n"
            + "mouseb_forward 2\r\n"
            + "use_joystick 0\r\n"
            + "screenblocks 9\r\n"
            + "detaillevel 0\r\n"
            + "snd_channels 4\r\n"
            + "snd_musicdevice 0\r\n"
            + "snd_sfxdevice 3\r\n"
            + "snd_sbport 544\r\n"
            + "snd_sbirq 7\r\n"
            + "snd_sbdma 1\r\n"
            + "snd_mport 816\r\n"
            + "usegamma 0\r\n";
    }
}
