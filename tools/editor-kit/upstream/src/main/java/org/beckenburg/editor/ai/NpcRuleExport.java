package org.beckenburg.editor.ai;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Objects;

/**
 * Deterministic, data-only export of the currently loaded canonical NPC rulebook.
 * No arbitrary serialization, hidden host state or extra JSON dependency.
 */
public final class NpcRuleExport {
    private NpcRuleExport(){}
    private static String quote(String raw){
        StringBuilder b=new StringBuilder("\"");
        for(int i=0;i<raw.length();i++){
            char c=raw.charAt(i);
            switch(c){
                case '"' -> b.append("\\\"");
                case '\\' -> b.append("\\\\");
                case '\n' -> b.append("\\n");
                case '\r' -> b.append("\\r");
                case '\t' -> b.append("\\t");
                default -> {
                    if(c<32)b.append(String.format(java.util.Locale.ROOT,"\\u%04x",(int)c));
                    else b.append(c);
                }
            }
        }
        return b.append('"').toString();
    }
    private static void field(StringBuilder b,String key,String value){
        b.append(quote(key)).append(':').append(quote(value));
    }
    private static void field(StringBuilder b,String key,int value){
        b.append(quote(key)).append(':').append(value);
    }
    private static void field(StringBuilder b,String key,boolean value){
        b.append(quote(key)).append(':').append(value);
    }
    public static String toJson(NpcRuleBook book){
        Objects.requireNonNull(book);
        StringBuilder b=new StringBuilder("{\"schema\":\"beckenburg.npc-canonical-rule-export/v1\",");
        b.append("\"note\":\"Source-derived developer snapshot; evidence is possible, never observed; no live hunter state\",");
        b.append("\"species\":[");
        List<NpcRuleBook.Rule> all=new ArrayList<>(book.all());
        all.sort(Comparator.comparing(NpcRuleBook.Rule::id));
        for(int i=0;i<all.size();i++){
            if(i>0)b.append(',');
            NpcRuleBook.Rule r=all.get(i);
            b.append('{');
            field(b,"id",r.id());b.append(',');
            field(b,"baseId",r.baseId());b.append(',');
            field(b,"classification",r.classification());b.append(',');
            field(b,"rite",r.rite());b.append(',');
            field(b,"behavior",r.behavior());b.append(',');
            field(b,"locomotion",r.locomotion());b.append(',');
            field(b,"passage",r.passage());b.append(',');
            field(b,"sight",r.sight());b.append(',');
            field(b,"charge",r.charge());b.append(',');
            field(b,"chargePeriod",r.chargePeriod());b.append(',');
            field(b,"movementPercent",r.movementPercent());b.append(',');
            field(b,"solitary",r.solitary());b.append(',');
            field(b,"leavesUv",r.leavesUv());b.append(',');
            field(b,"favoredAction",r.favoredAction());b.append(',');
            field(b,"aiPeriod",r.aiPeriod());b.append(',');
            field(b,"tell",r.tell());b.append(',');
            field(b,"clue",r.clue());b.append(',');
            field(b,"frenzyEnergyCost",r.frenzyEnergyCost());b.append(',');
            field(b,"frenzyEarliestTick",r.frenzyEarliestTick());b.append(',');
            b.append("\"frenzyAttacks\":[");
            var attacks=new ArrayList<>(r.frenzyAttacks());attacks.sort(String::compareTo);
            for(int k=0;k<attacks.size();k++){if(k>0)b.append(',');b.append(quote(attacks.get(k)));}
            b.append("],\"evidence\":{");
            var keys=new ArrayList<>(r.evidence().keySet());keys.sort(String::compareTo);
            for(int k=0;k<keys.size();k++){
                if(k>0)b.append(',');
                field(b,keys.get(k),r.evidence().get(keys.get(k)));
            }
            b.append("}}");
        }
        b.append("],\"diagnostics\":[");
        List<NpcRuleBook.Finding> findings=book.inspect();
        for(int i=0;i<findings.size();i++){
            if(i>0)b.append(',');
            NpcRuleBook.Finding f=findings.get(i);
            b.append('{');
            field(b,"code",f.code());b.append(',');
            field(b,"subject",f.subject());b.append(',');
            field(b,"detail",f.detail());
            b.append('}');
        }
        b.append("]}\n");
        return b.toString();
    }
    public static void save(NpcRuleBook book,Path path)throws IOException{
        Path directory=path.toAbsolutePath().getParent();
        Files.createDirectories(directory);
        Path pending=Files.createTempFile(directory,"npc-rules-", ".pending");
        try{
            Files.writeString(pending,toJson(book),StandardCharsets.UTF_8);
            try{Files.move(pending,path,StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);}
            catch(java.nio.file.AtomicMoveNotSupportedException e){
                Files.move(pending,path,StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(pending);}
    }
}
