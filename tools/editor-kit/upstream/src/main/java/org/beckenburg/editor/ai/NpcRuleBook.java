package org.beckenburg.editor.ai;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

/**
 * Game-agnostic, immutable, typed AI rule inspection contract.
 * Game adapters READ authoritative rule objects; imported text never executes code.
 */
public final class NpcRuleBook {
    public record Rule(String id,String baseId,String classification,String rite,
            String behavior,String locomotion,String passage,String sight,String charge,
            int chargePeriod,int movementPercent,boolean solitary,boolean leavesUv,
            String favoredAction,int aiPeriod,String tell,String clue,
            int frenzyEnergyCost,int frenzyEarliestTick,Set<String> frenzyAttacks,
            Map<String,String> evidence) {
        public Rule {
            if(!key(id)||!key(baseId)||!key(classification)||!key(rite)||!key(behavior)
                    ||!key(locomotion)||!key(passage)||!key(sight)||!key(charge)
                    ||!key(favoredAction)||chargePeriod<1||movementPercent<1
                    ||aiPeriod<4||frenzyEnergyCost<0||frenzyEarliestTick<0)
                throw new IllegalArgumentException("Invalid NPC species contract");
            tell=Objects.requireNonNull(tell);
            clue=Objects.requireNonNull(clue);
            if(tell.isBlank()||clue.isBlank()||tell.length()>256||clue.length()>512)
                throw new IllegalArgumentException("Missing or excessive description");
            frenzyAttacks=Set.copyOf(frenzyAttacks);
            evidence=Map.copyOf(evidence);
            for(String a:frenzyAttacks)if(!key(a))throw new IllegalArgumentException("Invalid attack name");
            for(var e:evidence.entrySet())
                if(!key(e.getKey())||e.getValue()==null||e.getValue().length()>256)
                    throw new IllegalArgumentException("Invalid evidence field");
        }
    }
    public record Finding(String code,String subject,String detail) {}
    private final Map<String,Rule> byId;
    public NpcRuleBook(List<Rule> rules) {
        Objects.requireNonNull(rules);
        if(rules.isEmpty()||rules.size()>512)throw new IllegalArgumentException("Expected 1..512 authored NPC rules");
        Map<String,Rule> result=new LinkedHashMap<>();
        for(Rule rule:rules){
            Objects.requireNonNull(rule,"Null rule");
            if(result.putIfAbsent(rule.id(),rule)!=null)
                throw new IllegalArgumentException("Duplicate rule id: "+rule.id());
        }
        byId=Map.copyOf(result);
    }
    private static boolean key(String s){
        return s!=null&&s.matches("[A-Za-z][A-Za-z0-9_.-]{0,79}");
    }
    public List<Rule> all(){return List.copyOf(byId.values());}
    public Rule get(String id){
        Rule rule=byId.get(id);
        if(rule==null)throw new IllegalArgumentException("Unknown NPC: "+id);
        return rule;
    }
    public List<Finding> inspect(){
        List<Finding> result=new ArrayList<>();
        for(Rule rule:all()){
            if(!byId.containsKey(rule.baseId()))
                result.add(new Finding("MISSING_BASE",rule.id(),"Base species not registered: "+rule.baseId()));
            else if(!rule.id().equals(rule.baseId())
                    &&!byId.get(rule.baseId()).id().equals(byId.get(rule.baseId()).baseId()))
                result.add(new Finding("NESTED_VARIANT",rule.id(),"Variant should point at base identity"));
            if(rule.charge().equals("ISOLATION")&&rule.solitary())
                result.add(new Finding("AUTHOR_NOTE",rule.id(),
                    "Isolation recharge and solitary-target preference are distinct conditions"));
            if(rule.sight().equals("CAMERA_ONLY"))
                result.add(new Finding("VISIBILITY_RULE",rule.id(),
                    "Camera-only visuals must not be sent through ordinary visible actor rendering"));
            if(rule.passage().equals("WALLS")&&rule.locomotion().equals("WALK"))
                result.add(new Finding("REVIEW_MOVEMENT",rule.id(),
                    "Wall-phase with walking movement needs collision policy review"));
            if(rule.favoredAction().equals("DRIFT"))
                result.add(new Finding("MOVEMENT_NOT_EVENT",rule.id(),
                    "DRIFT does not reset the host's haunting event cadence"));
        }
        return List.copyOf(result);
    }
    /** Plain-text explanation is generated from typed rules, never scraped from a PDF. */
    public String explain(String id){
        Rule r=get(id);
        StringBuilder s=new StringBuilder();
        s.append(r.id()).append(" [").append(r.classification()).append("] ");
        if(!r.id().equals(r.baseId()))s.append("inherits mechanical base ").append(r.baseId());
        s.append("\nBehavior: ").append(r.behavior()).append(" / ").append(r.tell());
        s.append("\nTraversal: ").append(r.locomotion()).append(" via ").append(r.passage())
            .append(" / movement ").append(r.movementPercent()).append("%");
        s.append("\nVisual channel: ").append(r.sight())
            .append(" / UV traces: ").append(r.leavesUv());
        s.append("\nEnergy: ").append(r.charge()).append(" each ").append(r.chargePeriod())
            .append(" eligible 100ms ticks");
        s.append("\nHunting: ").append(r.solitary()?"solitary target required for gated attacks":
            "not restricted to solitary target");
        s.append("\nFavored event: ").append(r.favoredAction())
            .append(" / AI bias period ").append(r.aiPeriod());
        s.append("\nFrenzy: cost ").append(r.frenzyEnergyCost())
            .append(", earliest tick ").append(r.frenzyEarliestTick())
            .append(", permitted attacks ").append(r.frenzyAttacks());
        s.append("\nRite: ").append(r.rite()).append("\nClue: ").append(r.clue());
        s.append("\nEvidence possibilities (NOT observed/confirmed): ").append(r.evidence());
        return s.toString();
    }
}
