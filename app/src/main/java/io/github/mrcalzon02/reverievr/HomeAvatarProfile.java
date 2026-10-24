package io.github.mrcalzon02.reverievr;

/** Versioned local cosmetic identity; no game-stat or online dependencies. */
final class HomeAvatarProfile {
    static final String[] THEMES={"Everyday Dreamer","Astral Ranger","Forest Warden",
        "Deep-Sea Diver","Clockwork Robot","Candy Admiral","Moon Witch",
        "Pixel Paladin","Noir Sleuth","Friendly Phantom","Retro Racer","Cozy Critter"};
    static final String[] COLORS={"Sky Blue","Sunset Orange","Moss Green",
        "Plum Purple","Rose Pink","Ocean Teal","Gold","Midnight"};
    static final float[][] RGB={{.25f,.64f,.84f},{.92f,.49f,.28f},
        {.35f,.65f,.37f},{.60f,.40f,.76f},{.91f,.48f,.64f},
        {.22f,.75f,.70f},{.96f,.78f,.30f},{.30f,.36f,.53f}};
    static final String[] HATS={"None","Explorer Cap","Wizard Hat",
        "Antennae","Animal Ears","Top Hat","Crown","Halo"};
    static final String[] EXTRAS={"None","Cape","Sash","Badge",
        "Wings","Utility Belt","Star Emblem","Bow Tie"};
    static final String[] EYES={"Bright","Sleepy","Cyber Glow","Starry"};
    static final HomeAvatarProfile DEFAULT=new HomeAvatarProfile(0,0,0,0,0);
    final int theme,color,hat,extra,eyes;
    HomeAvatarProfile(int t,int c,int h,int x,int e) {
        theme=valid(t,THEMES.length); color=valid(c,COLORS.length);
        hat=valid(h,HATS.length); extra=valid(x,EXTRAS.length);
        eyes=valid(e,EYES.length);
    }
    private static int valid(int value,int count) {
        if(value<0||value>=count)throw new IllegalArgumentException("Invalid avatar");
        return value;
    }
    HomeAvatarProfile step(int field,int dir) {
        switch(field) {
            case 0:
                int t=Math.floorMod(theme+dir,THEMES.length);
                return new HomeAvatarProfile(t,t%COLORS.length,
                    new int[]{0,1,4,1,3,6,2,6,5,7,1,4}[t],
                    new int[]{0,4,1,5,6,2,6,3,2,4,5,7}[t],eyes);
            case 1:return new HomeAvatarProfile(theme,Math.floorMod(color+dir,COLORS.length),hat,extra,eyes);
            case 2:return new HomeAvatarProfile(theme,color,Math.floorMod(hat+dir,HATS.length),extra,eyes);
            case 3:return new HomeAvatarProfile(theme,color,hat,Math.floorMod(extra+dir,EXTRAS.length),eyes);
            case 4:return new HomeAvatarProfile(theme,color,hat,extra,Math.floorMod(eyes+dir,EYES.length));
            default:throw new IllegalArgumentException("Unknown avatar field");
        }
    }
    String encode() { return "v1:"+theme+":"+color+":"+hat+":"+extra+":"+eyes; }
    static HomeAvatarProfile decode(String saved) {
        if(saved==null)return DEFAULT;
        String[] fields=saved.split(":",-1);
        if(fields.length!=6||!"v1".equals(fields[0]))return DEFAULT;
        try {
            return new HomeAvatarProfile(Integer.parseInt(fields[1]),
                Integer.parseInt(fields[2]),Integer.parseInt(fields[3]),
                Integer.parseInt(fields[4]),Integer.parseInt(fields[5]));
        } catch(IllegalArgumentException malformed) { return DEFAULT; }
    }
}
