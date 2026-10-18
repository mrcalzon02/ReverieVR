package io.github.mrcalzon02.reverievr.game.breakwater;

/** Deterministic tiny PCM16 gameplay cues, generated once without audio assets. */
final class NativeCombatWaveform {
    static final int SAMPLE_RATE = 22050;
    static final int FIRE = 0, RELOAD = 1, READY = 2;
    static final int ARTILLERY = 3, AIRSTRIKE = 4;
    // Byte-sized native ordered sound ids 1–5 map to this private bank
    // without changing the generic shell feedback/voice cue indices.
    static final int MACHINE = 5, HEAVY = 6, FLAK = 7;
    static final int MISSILE = 8, CANNON = 9;
    static final int HULL = 10, SPLASH = 11, BOAT_DOWN = 12;
    static final int GUNBOAT = 13, VOLLEY = 14, BOMB = 15;
    static final int STRAFE = 16, SAM = 17, WAVE = 18, COLLISION = 19;
    static final int PURCHASE = 20, CLEAR = 21, VICTORY = 22, DEFEAT = 23;
    static final int ARMORED_GUN = 24, ARMOR_DESTROYED = 25;
    static final int COUNT = 26;

    private NativeCombatWaveform() {}

    // The native ABI uses ordered cue IDs 1..5 (not the PCM bank's
    // integer constants 5..9). This mapping is pure JVM-testable.
    static int cueForNativeEvent(int id) {
        // The JNI queue transmits 1-byte ordered cue IDs. The first five are
        // unchanged, followed by ten battle, four progression and two armor cues.
        return id>=1 && id<=21 ? MACHINE+id-1 : -1;
    }
    static int weaponCueForNativeEvent(int id) {
        return id>=1 && id<=5 ? cueForNativeEvent(id) : -1;
    }

    // Two overlapping reports are enough for the upgraded 160ms gun cycle;
    // all other cues retain one preloaded static voice.
    static int voiceCount(int cue) {
        if (cue < 0 || cue >= COUNT) throw new IllegalArgumentException("Unknown cue");
        return cue == FIRE || cue == MACHINE ? 2 : 1;
    }

    static int durationMillis(int cue) {
        if (cue == FIRE) return 180;
        if (cue == RELOAD) return 230;
        if (cue == READY) return 100;
        if (cue == ARTILLERY) return 520;
        if (cue == AIRSTRIKE) return 650;
        if (cue == MACHINE) return 170;
        if (cue == HEAVY) return 240;
        if (cue == FLAK) return 270;
        if (cue == MISSILE) return 390;
        if (cue == CANNON) return 360;
        if (cue == HULL) return 230;
        if (cue == SPLASH) return 340;
        if (cue == BOAT_DOWN) return 570;
        if (cue == GUNBOAT) return 410;
        if (cue == VOLLEY) return 295;
        if (cue == BOMB) return 610;
        if (cue == STRAFE) return 340;
        if (cue == SAM) return 460;
        if (cue == WAVE) return 390;
        if (cue == COLLISION) return 490;
        if (cue == PURCHASE) return 155;
        if (cue == CLEAR) return 1180;
        if (cue == VICTORY) return 770;
        if (cue == DEFEAT) return 690;
        if (cue == ARMORED_GUN) return 420;
        if (cue == ARMOR_DESTROYED) return 630;
        throw new IllegalArgumentException("Unknown cue");
    }

    static byte[] generatePcm16(int cue) {
        int count = SAMPLE_RATE * durationMillis(cue) / 1000;
        byte[] data = new byte[count * 2];
        int noiseState = 0x64ad017c ^ ((cue + 1) * 0x61c88647);
        double phase = 0.0;
        for (int i = 0; i < count; i++) {
            double t = i / (double) SAMPLE_RATE;
            double p = i / (double) (count - 1);
            noiseState ^= noiseState << 13;
            noiseState ^= noiseState >>> 17;
            noiseState ^= noiseState << 5;
            double noise = ((noiseState & 65535) / 32767.5) - 1.0;
            double envelope = Math.min(1.0, t * 900.0)
                * Math.pow(1.0 - p, 2.3);
            double signal;
            if (cue == FIRE) {
                phase += 2.0 * Math.PI * (90.0 +
                    220.0 * Math.exp(-22.0 * t)) / SAMPLE_RATE;
                signal = envelope * (0.48 * noise * Math.exp(-12.0 * t)
                    + 0.35 * Math.sin(phase) * Math.exp(-10.0 * t));
            } else if (cue >= MACHINE && cue <= CANNON) {
                // Distinct original synthesizer recipes, not a second
                // short gunshot played at five arbitrary playback speeds.
                // One-shot envelopes carry weight without sample packs.
                if (cue == MACHINE) {
                    phase += 2.0 * Math.PI * (122.0 +
                        245.0 * Math.exp(-27.0 * t)) / SAMPLE_RATE;
                    signal = envelope * (0.53 * noise * Math.exp(-17.0 * t)
                        + .32 * Math.sin(phase) * Math.exp(-12.0 * t));
                } else if (cue == HEAVY) {
                    phase += 2.0 * Math.PI * (57.0 +
                        172.0 * Math.exp(-16.0 * t)) / SAMPLE_RATE;
                    signal = envelope * (.43 * noise * Math.exp(-9.0 * t)
                        + .47 * Math.sin(phase) * Math.exp(-9.5 * t));
                } else if (cue == FLAK) {
                    phase += 2.0 * Math.PI * (86.0 +
                        300.0 * Math.exp(-30.0 * t)) / SAMPLE_RATE;
                    double snap = Math.exp(-52.0 * t);
                    signal = envelope * (.69 * noise * snap +
                        .28 * Math.sin(phase) * Math.exp(-14.0 * t));
                } else if (cue == MISSILE) {
                    phase += 2.0 * Math.PI * (470.0 -
                        310.0 * p) / SAMPLE_RATE;
                    double exhaust = Math.exp(-5.0 * t);
                    signal = envelope * exhaust *
                        (.39 * noise + .45 * Math.sin(phase));
                } else {
                    phase += 2.0 * Math.PI * (48.0 +
                        125.0 * Math.exp(-11.0 * t)) / SAMPLE_RATE;
                    signal = envelope * (.40 * noise * Math.exp(-8.0 * t)
                        + .51 * Math.sin(phase) * Math.exp(-7.0 * t));
                }
            } else if (cue >= HULL && cue <= COLLISION) {
                // Ten short original effects, synthesized only at audio-bank
                // construction: no network assets, per-frame allocations,
                // proprietary samples or second soundtrack player.
                final double attack = Math.exp(-8.0*t);
                if (cue == SPLASH) {
                    phase += 2*Math.PI*(80.0 + 35.0*p)/SAMPLE_RATE;
                    signal=envelope*(.52*noise*attack
                        +.18*Math.sin(phase)*Math.exp(-9.0*t));
                } else if (cue == HULL) {
                    phase += 2*Math.PI*(240.0-130.0*p)/SAMPLE_RATE;
                    signal=envelope*(.57*noise*Math.exp(-23.0*t)
                        +.40*Math.sin(phase)*attack);
                } else if (cue == BOAT_DOWN || cue == COLLISION ||
                           cue == BOMB) {
                    final double base=cue==BOMB?51.0:
                        cue==COLLISION?66.0:73.0;
                    phase+=2*Math.PI*(base+170.0*Math.exp(-17.0*t))/SAMPLE_RATE;
                    double rumble=Math.exp(-(cue==BOMB?5.0:7.5)*t);
                    double secondary=cue==BOMB && t>.17 ?
                        Math.exp(-22.0*(t-.17)):0.0;
                    signal=envelope*(.45*Math.sin(phase)*rumble+
                        .54*noise*Math.exp(-11.0*t)+secondary*.28*noise);
                } else if (cue == GUNBOAT) {
                    phase+=2*Math.PI*(77.0+105.0*Math.exp(-16.0*t))/SAMPLE_RATE;
                    signal=envelope*(.59*noise*Math.exp(-13.0*t)+
                        .42*Math.sin(phase)*Math.exp(-10.0*t));
                } else if (cue == VOLLEY || cue == STRAFE) {
                    final double spacing=cue==STRAFE?.095:.073;
                    double burst=Math.exp(-45.0*(t%spacing));
                    phase+=2*Math.PI*(cue==STRAFE?205.0:152.0)/SAMPLE_RATE;
                    signal=envelope*burst*(.59*noise+.29*Math.sin(phase));
                } else if (cue == SAM) {
                    phase+=2*Math.PI*(215.0+380.0*p)/SAMPLE_RATE;
                    signal=envelope*(.36*Math.sin(phase)+.44*noise)
                        *Math.exp(-4.0*t);
                } else {
                    // Wave-start signal is a restrained two-tone muster cue,
                    // not an additional musical bed over Mars des Souvenirs.
                    double freq=t<.17?523.25:659.25;
                    phase+=2*Math.PI*freq/SAMPLE_RATE;
                    signal=envelope*.44*Math.sin(phase);
                }
            } else if (cue == ARMORED_GUN || cue == ARMOR_DESTROYED) {
                if(cue==ARMORED_GUN) {
                    // Heavy breech crack and lower mechanical boom, quite
                    // different from an offshore gunboat's echo.
                    phase += 2.0*Math.PI*(61.0 +
                        190.0*Math.exp(-22.0*t))/SAMPLE_RATE;
                    signal=envelope*(.56*noise*Math.exp(-15.0*t)+
                        .49*Math.sin(phase)*Math.exp(-6.0*t));
                } else {
                    // Armor rupture and settling metallic debris.
                    phase += 2.0*Math.PI*(72.0+
                        120.0*Math.exp(-9.0*t))/SAMPLE_RATE;
                    double fragments=(t>.13 && t<.39)?
                        .15*noise*Math.exp(-7.0*(t-.13)):0.0;
                    signal=envelope*(.56*noise*Math.exp(-8.0*t)+
                        .34*Math.sin(phase)*Math.exp(-5.0*t)+fragments);
                }
            } else if (cue >= PURCHASE && cue <= DEFEAT) {
                // Four brief state-transition cues, intentionally distinct
                // from and quieter than the looping Mars des Souvenirs song.
                // No music asset is modified, restarted or mixed into PCM.
                if (cue == PURCHASE) {
                    phase += 2.0*Math.PI*(t<.065?520.0:780.0)/SAMPLE_RATE;
                    signal = envelope*.30*Math.sin(phase)
                        + envelope*.09*noise*Math.exp(-45.0*t);
                } else if (cue == CLEAR) {
                    // A short, recognizable four-note brass-like fanfare:
                    // G4-B4-D5-G5 with a held final chord. The note attack
                    // and sustain are independent of the generic SFX envelope.
                    final double frequency=t<.18?392.00:
                        t<.36?493.88:t<.55?587.33:783.99;
                    final double noteStart=t<.18?0.0:
                        t<.36?.18:t<.55?.36:.55;
                    final double local=t-noteStart;
                    final double attack=Math.min(1.0,local*58.0);
                    final double tail=Math.min(1.0,
                        Math.max(0.0,(1.18-t)/.20));
                    phase += 2.0*Math.PI*frequency/SAMPLE_RATE;
                    signal=.38*attack*tail*(.75*Math.sin(phase)+
                        .17*Math.sin(2.0*phase)+.08*Math.sin(3.0*phase));
                    if(t>=.55) {
                        // Quiet chord below the held tonic resolves the cue.
                        signal+=.09*tail*Math.sin(2.0*Math.PI*392.0*t)
                            +.07*tail*Math.sin(2.0*Math.PI*587.33*t);
                    }
                } else if (cue == VICTORY) {
                    final double frequency = t<.19?392.0:
                        t<.38?494.0:t<.56?587.0:784.0;
                    phase += 2.0*Math.PI*frequency/SAMPLE_RATE;
                    signal = envelope*.34*Math.sin(phase)
                        + envelope*.08*Math.sin(phase*1.5);
                } else {
                    phase += 2.0*Math.PI*(210.0 - 125.0*p)/SAMPLE_RATE;
                    signal = envelope*(.27*Math.sin(phase)
                        + .11*noise*Math.exp(-9.0*t));
                }
            } else if (cue == RELOAD) {
                double clack = Math.exp(-58.0 * t) + (t >= 0.115 ?
                    .85 * Math.exp(-70.0 * (t - 0.115)) : 0.0);
                signal = envelope * clack * (0.32 * noise
                    + 0.32 * Math.sin(2.0 * Math.PI * 810.0 * t));
            } else if (cue == ARTILLERY) {
                // Three staggered low-frequency shells, distinct from the MG.
                double local = t % 0.17;
                double impact = Math.exp(-30.0 * local);
                signal = envelope * impact * (0.46 * noise
                    + 0.46 * Math.sin(2.0 * Math.PI * 72.0 * local));
            } else if (cue == AIRSTRIKE) {
                // Descending engine whistle followed by an overhead blast.
                phase += 2.0 * Math.PI * (210.0 - 140.0 * p) / SAMPLE_RATE;
                double blast = t >= 0.30 ? Math.exp(-16.0 * (t - 0.30)) : 0.0;
                signal = envelope * (0.42 * Math.sin(phase) * (1.0 - p)
                    + blast * (0.62 * noise +
                        0.30 * Math.sin(2.0 * Math.PI * 65.0 * (t - 0.30))));
            } else {
                signal = envelope * Math.exp(-30.0 * t) *
                    (0.24 * noise + 0.45 * Math.sin(2.0 * Math.PI * 1060.0 * t));
            }
            int sample = (int) Math.round(Math.max(-1.0,
                Math.min(1.0, signal)) * 32767.0);
            data[2 * i] = (byte) sample;
            data[2 * i + 1] = (byte) (sample >>> 8);
        }
        return data;
    }
}
