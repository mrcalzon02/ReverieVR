package io.github.mrcalzon02.reverievr;

import java.util.Random;

/**
 * ReverieVR avatar identity, version 2. Presentation, silhouette and costume
 * are independent: no body size, outfit or body proportions are gender-gated.
 * A v1 saved avatar migrates losslessly for all five original cosmetic fields.
 */
final class HomeAvatarProfile {
    static final String[] THEMES={"Everyday Dreamer", "Astral Ranger", "Forest Warden", "Deep-Sea Diver", "Clockwork Robot", "Candy Admiral", "Moon Witch", "Pixel Paladin", "Noir Sleuth", "Friendly Phantom", "Retro Racer", "Cozy Critter", "Soviet Strongman", "Iron Valkyrie", "Neon Samurai", "Desert Nomad", "Frost Monarch", "Gothic Aristocrat", "Starship Engineer", "Cyberpunk Courier", "Swamp Alchemist", "Highland Guardian", "Arctic Explorer", "Venetian Masquerade", "Celestial Oracle", "Solar Knight", "Haunted Doll", "Frontier Gunslinger", "Pirate Captain", "Royal Musketeer", "Clockwork Gentleman", "Art Deco Diva", "Treetop Rogue", "Undersea Oracle", "Volcanic Smith", "Rainbow Jester", "Galactic Diplomat", "Wasteland Survivor", "Bog Witch", "Cosmic Dancer", "Retro Astronaut", "Victorian Inventor", "Crystal Sorcerer", "Velvet Vampire", "Flower Knight", "Mechanist Pilgrim", "Heavy Mecha Pilot", "Mythic Giant"};
    static final String[] COLORS={"Sky Blue", "Sunset Orange", "Moss Green", "Plum Purple", "Rose Pink", "Ocean Teal", "Gold", "Midnight", "Maroon", "Ice Silver", "Brass", "Ivory", "Obsidian", "Lavender", "Copper", "Emerald"};
    static final float[][] RGB={
        {0.25f,0.64f,0.84f},
        {0.92f,0.49f,0.28f},
        {0.35f,0.65f,0.37f},
        {0.60f,0.40f,0.76f},
        {0.91f,0.48f,0.64f},
        {0.22f,0.75f,0.70f},
        {0.96f,0.78f,0.30f},
        {0.30f,0.36f,0.53f},
        {0.64f,0.17f,0.24f},
        {0.80f,0.87f,0.91f},
        {0.75f,0.56f,0.24f},
        {0.95f,0.91f,0.78f},
        {0.16f,0.17f,0.22f},
        {0.76f,0.64f,0.95f},
        {0.78f,0.42f,0.22f},
        {0.09f,0.54f,0.34f}
    };
    static final String[] HATS={"None", "Explorer Cap", "Wizard Hat", "Antennae", "Animal Ears", "Top Hat", "Crown", "Halo", "Laurel Wreath", "Knight Helm", "Pirate Tricorn", "Newsboy Cap", "Travel Hood", "Royal Circlet", "Cyber Visor", "Flower Crown"};
    static final String[] EXTRAS={"None", "Cape", "Sash", "Badge", "Wings", "Utility Belt", "Star Emblem", "Bow Tie", "Shoulder Pauldrons", "Satchel", "Long Scarf", "Goggles", "Mechanical Arms", "Feather Mantle", "Medallion", "Backpack"};
    static final String[] EYES={"Bright","Sleepy","Cyber Glow","Starry"};
    static final String[] PRESENTATIONS={"Feminine", "Masculine", "Neutral"};
    static final String[] PHYSIQUES={"Petite", "Lithe", "Compact", "Balanced", "Athletic", "Curvy", "Powerful", "Stocky", "Heavyset", "Broad", "Colossal", "Willowy"};
    static final String[] OUTFITS={"Casual Layers", "Explorer Jacket", "Arcane Robes", "Ceremonial Dress", "Heavy Plate", "Mechanic Coveralls", "Naval Uniform", "Military Greatcoat", "Formal Suit", "Ball Gown", "Streetwear", "Courtly Tunic", "Gothic Coat", "Space Suit", "Nomad Wraps", "Racing Leathers", "Warden Cloak", "Samurai Armor", "Winter Parka", "Cottage Dress", "Painter Smock", "Royal Regalia", "Steampunk Vest", "Festival Costume"};
    static final String[] HAIR={"Shaved", "Pixie Cut", "Short Crop", "Bob", "Long Flowing", "Curly", "Braids", "Ponytail", "Top Bun", "Mohawk", "Side Swept", "Wild Spikes"};
    static final String[] FABRICS=COLORS;
    static final String[] DEGREES={"Minimum","Low","Light","Medium","Strong","High","Maximum"};
    // Preset sequence: height, shoulders, waist, hips, muscle. All editable.
    static final int[][] BODY_PRESETS={
        {1, 1, 1, 2, 1},
        {4, 1, 0, 1, 1},
        {1, 3, 3, 3, 2},
        {3, 3, 3, 3, 3},
        {4, 4, 2, 3, 5},
        {3, 2, 3, 5, 2},
        {4, 5, 4, 4, 6},
        {1, 5, 5, 5, 4},
        {3, 4, 6, 6, 2},
        {4, 6, 4, 4, 5},
        {6, 6, 6, 6, 6},
        {6, 1, 0, 2, 0}
    };
    static final int[][] PAGE_FIELDS={{0,1,5,6,4},{7,8,9,10,11},{12,2,3,13,14}};
    static final String[] PAGE_NAMES={"IDENTITY","BODY PROPORTIONS","OUTFIT & DETAILS"};
    static final String[] FIELD_NAMES={
        "IDENTITY","ACCENT COLOR","HEADWEAR","ACCESSORY","EYES",
        "PRESENTATION","BODY PRESET","HEIGHT","SHOULDERS","WAIST",
        "HIPS","MUSCULATURE","OUTFIT","HAIRSTYLE","FABRIC COLOR"
    };
    static final HomeAvatarProfile DEFAULT=new HomeAvatarProfile(
        0,0,0,0,0,2,3,3,3,3,3,3,0,2,0);

    final int theme,color,hat,extra,eyes,presentation,physique;
    final int height,shoulders,waist,hips,muscle,outfit,hair,fabric;

    HomeAvatarProfile(int t,int c,int h,int x,int e) {
        this(t,c,h,x,e,2,3,3,3,3,3,3,0,2,0);
    }
    HomeAvatarProfile(int t,int c,int h,int x,int e,int pr,int ph,
        int ht,int sh,int wa,int hip,int mus,int outfit,int hair,int fabric) {
        theme=valid(t,THEMES.length); color=valid(c,COLORS.length);
        hat=valid(h,HATS.length); extra=valid(x,EXTRAS.length);
        eyes=valid(e,EYES.length);presentation=valid(pr,PRESENTATIONS.length);
        physique=valid(ph,PHYSIQUES.length);height=valid(ht,7);
        shoulders=valid(sh,7);waist=valid(wa,7);hips=valid(hip,7);
        muscle=valid(mus,7);this.outfit=valid(outfit,OUTFITS.length);
        this.hair=valid(hair,HAIR.length);this.fabric=valid(fabric,FABRICS.length);
    }
    private static int valid(int value,int count) {
        if(value<0||value>=count)throw new IllegalArgumentException("Invalid avatar selection");
        return value;
    }
    int get(int field) {
        switch(field) {
            case 0:return theme;case 1:return color;case 2:return hat;
            case 3:return extra;case 4:return eyes;case 5:return presentation;
            case 6:return physique;case 7:return height;case 8:return shoulders;
            case 9:return waist;case 10:return hips;case 11:return muscle;
            case 12:return outfit;case 13:return hair;case 14:return fabric;
            default:throw new IllegalArgumentException("Unknown avatar field");
        }
    }
    static int count(int field) {
        switch(field) {
            case 0:return THEMES.length;case 1:return COLORS.length;
            case 2:return HATS.length;case 3:return EXTRAS.length;
            case 4:return EYES.length;case 5:return PRESENTATIONS.length;
            case 6:return PHYSIQUES.length;
            case 7:case 8:case 9:case 10:case 11:return 7;
            case 12:return OUTFITS.length;case 13:return HAIR.length;
            case 14:return FABRICS.length;
            default:throw new IllegalArgumentException("Unknown avatar field");
        }
    }
    String fieldLabel(int field) {
        switch(field) {
            case 0:return THEMES[theme];case 1:return COLORS[color];
            case 2:return HATS[hat];case 3:return EXTRAS[extra];
            case 4:return EYES[eyes];case 5:return PRESENTATIONS[presentation];
            case 6:return PHYSIQUES[physique];
            case 7:case 8:case 9:case 10:case 11:return DEGREES[get(field)];
            case 12:return OUTFITS[outfit];case 13:return HAIR[hair];
            case 14:return FABRICS[fabric];
            default:throw new IllegalArgumentException("Unknown avatar field");
        }
    }
    HomeAvatarProfile step(int field,int delta) {
        int[] v=fields();
        v[field]=Math.floorMod(v[field]+delta,count(field));
        if(field==0) {
            // Themes suggest an ensemble but NEVER overwrite body or identity.
            v[12]=v[0]%OUTFITS.length;
            v[2]=v[0]%HATS.length;
            v[3]=v[0]%EXTRAS.length;
        } else if(field==6) {
            for(int i=0;i<5;i++)v[7+i]=BODY_PRESETS[v[6]][i];
        }
        return of(v);
    }
    /** Fixed snapshot order shared with the append-only native AvatarV1 ABI. */
    int[] forNativeModules() { return fields(); }
    private int[] fields() {
        return new int[]{theme,color,hat,extra,eyes,presentation,physique,
            height,shoulders,waist,hips,muscle,outfit,hair,fabric};
    }
    private static HomeAvatarProfile of(int[] v) {
        return new HomeAvatarProfile(v[0],v[1],v[2],v[3],v[4],v[5],v[6],
            v[7],v[8],v[9],v[10],v[11],v[12],v[13],v[14]);
    }
    static HomeAvatarProfile random(Random random) {
        if(random==null)throw new IllegalArgumentException("Random source missing");
        int preset=random.nextInt(PHYSIQUES.length);
        int[] b=BODY_PRESETS[preset];
        return new HomeAvatarProfile(random.nextInt(THEMES.length),
            random.nextInt(COLORS.length),random.nextInt(HATS.length),
            random.nextInt(EXTRAS.length),random.nextInt(EYES.length),
            random.nextInt(PRESENTATIONS.length),preset,
            b[0],b[1],b[2],b[3],b[4],random.nextInt(OUTFITS.length),
            random.nextInt(HAIR.length),random.nextInt(FABRICS.length));
    }
    float heightScale() { return .64f+.04f*height; }
    float horizontalScale() { return .82f+.06f*hips; }
    boolean sameGeometry(HomeAvatarProfile o) {
        return o!=null && shoulders==o.shoulders && waist==o.waist
            && hips==o.hips && muscle==o.muscle
            && presentation==o.presentation && outfit==o.outfit
            && hair==o.hair && height==o.height;
    }
    String encode() {
        StringBuilder b=new StringBuilder("v2");
        for(int x:fields())b.append(':').append(x);
        return b.toString();
    }
    static HomeAvatarProfile decode(String saved) {
        if(saved==null)return DEFAULT;
        String[] fields=saved.split(":",-1);
        try {
            if(fields.length==6&&"v1".equals(fields[0])) {
                return new HomeAvatarProfile(Integer.parseInt(fields[1]),
                    Integer.parseInt(fields[2]),Integer.parseInt(fields[3]),
                    Integer.parseInt(fields[4]),Integer.parseInt(fields[5]));
            }
            if(fields.length!=16||!"v2".equals(fields[0]))return DEFAULT;
            int[] v=new int[15];
            for(int i=0;i<v.length;i++)v[i]=Integer.parseInt(fields[i+1]);
            return of(v);
        } catch(IllegalArgumentException invalid) {return DEFAULT;}
    }
}
