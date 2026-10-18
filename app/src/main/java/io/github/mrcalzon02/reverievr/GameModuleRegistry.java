package io.github.mrcalzon02.reverievr;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

/**
 * One allowlisted Java identity registry for game-content downloads, Stage A
 * library and independently signed companion APKs. The native C ABI host
 * list remains compiled/verified separately until companion migration.
 */
final class GameModuleRegistry {
    static final class Entry {
        final String id,displayName,contentNamespace,companionPackage;
        final boolean developmentOnly;
        Entry(String id,String displayName,String contentNamespace,
              String companionPackage,boolean developmentOnly){
            this.id=id;this.displayName=displayName;
            this.contentNamespace=contentNamespace;
            this.companionPackage=companionPackage;
            this.developmentOnly=developmentOnly;
        }
    }
    static final Entry[] GAMES={
        new Entry("between-deliveries-red-ledger","Red Ledger","red-ledger",
            "io.github.mrcalzon02.reverievr.game.redledger",true),
        new Entry("breakwater-battery","Breakwater Battery","breakwater",
            "io.github.mrcalzon02.reverievr.game.breakwater",true),
        new Entry("omega-front","Omega Front","omega-front",
            "io.github.mrcalzon02.reverievr.game.omegafront",true),
        new Entry("gumball-armada","Gumball Armada","gumball-armada",
            "io.github.mrcalzon02.reverievr.game.gumballarmada",true),
        new Entry("ministry-lantern-desk","Ministry of Intelligence","lantern-desk",
            "io.github.mrcalzon02.reverievr.game.ministry",true),
        new Entry("iron-sight-forward-detachment","Forward Detachment","forward-detachment",
            "io.github.mrcalzon02.reverievr.game.forwarddetachment",true),
        new Entry("classic-chess","Chess","classic-chess",
            "io.github.mrcalzon02.reverievr.game.classicchess",false),
        new Entry("classic-go","Go","classic-go",
            "io.github.mrcalzon02.reverievr.game.classicgo",false),
        new Entry("classic-checkers","Checkers","classic-checkers",
            "io.github.mrcalzon02.reverievr.game.classiccheckers",false),
        new Entry("classic-pong","Pong","classic-pong",
            "io.github.mrcalzon02.reverievr.game.classicpong",false),
        new Entry("classic-snake","Snake","classic-snake",
            "io.github.mrcalzon02.reverievr.game.classicsnake",false),
        new Entry("classic-reversi","Reversi","classic-reversi",
            "io.github.mrcalzon02.reverievr.game.classicreversi",false),
        new Entry("classic-breakout","Breakout","classic-breakout",
            "io.github.mrcalzon02.reverievr.game.classicbreakout",false),
        new Entry("classic-chutes-ladders","Chutes & Ladders","classic-chutes-ladders",
            "io.github.mrcalzon02.reverievr.game.classicchutesladders",false),
        new Entry("procedural-test-chamber","Test Chamber Companion (Test)","test-chamber",
            "io.github.mrcalzon02.reverievr.game.testchamber",true)
    };
    static {
        Set<String> ids=new HashSet<>(),packages=new HashSet<>(),namespaces=new HashSet<>();
        for(Entry e:GAMES){
            if(!e.id.matches("[a-z0-9][a-z0-9-]{2,79}") ||
               !e.contentNamespace.matches("[a-z0-9][a-z0-9-]{2,79}") ||
               !e.companionPackage.matches("[a-z0-9.]{10,160}") ||
               !ids.add(e.id) || !packages.add(e.companionPackage) ||
               !namespaces.add(e.contentNamespace))
                throw new IllegalStateException("Duplicate or invalid native game identity");
        }
    }
    static Entry find(String id){
        if(id==null)return null;
        for(Entry entry:GAMES)if(entry.id.equals(id))return entry;
        return null;
    }
    private GameModuleRegistry(){}
}
