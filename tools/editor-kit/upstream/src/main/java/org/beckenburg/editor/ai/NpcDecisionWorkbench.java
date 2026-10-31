package org.beckenburg.editor.ai;

import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * Read-only explanation/guardrail for an actor's ALREADY FILTERED host observation.
 * This never moves a body, reveals a hunter, authorizes evidence or dispatches an attack.
 */
public final class NpcDecisionWorkbench {
    public record Observation(int tick,int room,int energy,boolean hearsHunter,
            boolean manifested,List<Integer> exits,Set<String> legalActions) {
        public Observation {
            if(tick<0||room<0||energy<0||energy>100||exits==null||legalActions==null
                    ||exits.size()>512||legalActions.size()>32)
                throw new IllegalArgumentException("Invalid coarse NPC observation");
            exits=List.copyOf(exits);
            legalActions=Set.copyOf(legalActions);
            if(new LinkedHashSet<>(exits).size()!=exits.size())
                throw new IllegalArgumentException("Duplicate portal exits");
            for(int exit:exits)if(exit<0||exit==room)
                throw new IllegalArgumentException("Invalid portal exit");
            for(String action:legalActions)
                if(action==null||!action.matches("[A-Z][A-Z0-9_]{0,63}"))
                    throw new IllegalArgumentException("Invalid action name");
        }
    }
    public record Decision(String action,int targetRoom,boolean eligible,String reason) {
        public Decision {
            Objects.requireNonNull(action);
            Objects.requireNonNull(reason);
        }
    }
    public record Trace(int tick,String npc,Decision decision) {}
    private NpcDecisionWorkbench(){}
    public static Decision inspect(NpcRuleBook.Rule rule,Observation observed,
                                   String action,int targetRoom,boolean verifiedVictim){
        Objects.requireNonNull(rule);Objects.requireNonNull(observed);
        if(action==null||!action.matches("[A-Z][A-Z0-9_]{0,63}"))
            throw new IllegalArgumentException("Unknown action label");
        if(!observed.legalActions().contains(action))
            return new Decision(action,targetRoom,false,
                "Host observation did not authorize this action (energy, cadence, cooldown or state)");
        if(action.equals("DRIFT")){
            if(!observed.exits().contains(targetRoom))
                return new Decision(action,targetRoom,false,"Destination is not an authorized portal exit");
            return new Decision(action,targetRoom,true,
                "Host permits DRIFT through a listed exit. Movement remains subject to swept collision.");
        }
        if(targetRoom!=observed.room())
            return new Decision(action,targetRoom,false,"Non-movement actions must target the current room");
        if(action.equals("STRIKE")&&!verifiedVictim)
            return new Decision(action,targetRoom,false,
                "No host-verified spatial victim: never assume an audible target is in contact");
        if(action.equals("MANIFEST")&&!observed.hearsHunter())
            return new Decision(action,targetRoom,false,
                "No host hearing condition for manifestation");
        if(action.equals("SIGNATURE")&&!observed.hearsHunter())
            return new Decision(action,targetRoom,false,"Signature requires host-validated hearing");
        return new Decision(action,targetRoom,true,
            action.equals(rule.favoredAction())
                ?"Host action is legal and matches this species' preferred behavior; no event is dispatched"
                :"Host action is legal, though it is not this species' preferred event; no event is dispatched");
    }
    /** Preview proposal is advisory, not the production AI or the real choice RNG.
     * No omniscience: only coarse host-provided legal actions and a victim-contact bit.
     */
    public static Decision propose(NpcRuleBook.Rule rule,Observation observed,
                                   boolean verifiedVictim){
        Objects.requireNonNull(rule);Objects.requireNonNull(observed);
        List<String> preference=new ArrayList<>();
        if(observed.hearsHunter()){
            preference.add("STRIKE");preference.add("SIGNATURE");preference.add("MANIFEST");
        }
        preference.add(rule.favoredAction());
        preference.add("DISTURB");preference.add("WHISPER");
        preference.add("DRIFT");
        for(String action:new LinkedHashSet<>(preference)){
            if(!observed.legalActions().contains(action))continue;
            if(action.equals("DRIFT")){
                for(int target:observed.exits()){
                    Decision candidate=inspect(rule,observed,action,target,verifiedVictim);
                    if(candidate.eligible())return candidate;
                }
            }else{
                Decision candidate=inspect(rule,observed,action,observed.room(),verifiedVictim);
                if(candidate.eligible())return candidate;
            }
        }
        return new Decision("NONE",observed.room(),false,
            "No action passed the supplied host legality and victim-contact constraints");
    }
    /** Fixed-size deterministic replay. Pure analysis, never a world-driving loop. */
    public static List<Trace> replay(NpcRuleBook.Rule rule,List<Observation> input,
                                     boolean verifiedVictim){
        Objects.requireNonNull(input);
        if(input.size()>512)throw new IllegalArgumentException("Replay capped at 512 observations");
        int prior=-1;
        List<Trace> result=new ArrayList<>();
        for(Observation snapshot:input){
            if(snapshot.tick()<=prior)throw new IllegalArgumentException("Out-of-order replay tick");
            prior=snapshot.tick();
            result.add(new Trace(snapshot.tick(),rule.id(),propose(rule,snapshot,verifiedVictim)));
        }
        return List.copyOf(result);
    }
}
