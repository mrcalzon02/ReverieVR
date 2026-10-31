package org.beckenburg.editor.ai;

import java.util.Arrays;
import java.util.EnumMap;
import java.util.Map;

/**
 * Opt-in, bounded, per-host AI performance diagnostics.
 * Stores ONLY time durations and event counts, never rooms, coordinates,
 * player identity, evidence, actor health or network packets.
 * Production path remains zero-allocation when disabled.
 */
public final class NpcAiTelemetry {
    public enum Metric { HOST_TICK, PERCEPTION, DECISION, APPROACH, HEARING, ROUTE }
    public record Percentiles(long calls,long minimumNs,long p50Ns,long p95Ns,
                              long p99Ns,long maximumNs,long meanNs) {}
    public record Snapshot(boolean enabled,int windowSize,Map<Metric,Percentiles> timings) {
        public Snapshot {timings=Map.copyOf(timings);}
    }
    public static final int WINDOW=256;
    private static final long MAX_NS=60_000_000_000L;
    private static final class Series {
        final long[] recent=new long[WINDOW];
        int count,cursor;
        long totalCalls;
        void record(long ns){
            recent[cursor]=ns;
            cursor=(cursor+1)%WINDOW;
            if(count<WINDOW)count++;
            totalCalls++;
        }
        Percentiles snapshot(){
            if(count==0)return new Percentiles(totalCalls,0,0,0,0,0,0);
            long[] sorted=Arrays.copyOf(recent,count);
            Arrays.sort(sorted);
            long sum=0;
            for(long n:sorted)sum+=n;
            return new Percentiles(totalCalls,sorted[0],percentile(sorted,.50),
                percentile(sorted,.95),percentile(sorted,.99),
                sorted[count-1],sum/count);
        }
        static long percentile(long[] data,double p){
            return data[Math.max(0,Math.min(data.length-1,(int)Math.ceil(p*data.length)-1))];
        }
    }
    private final EnumMap<Metric,Series> recorded=new EnumMap<>(Metric.class);
    private volatile boolean enabled;
    public NpcAiTelemetry(){for(Metric m:Metric.values())recorded.put(m,new Series());}
    /** Host-side developer switch only. No client or packet control. */
    public void setEnabled(boolean value){enabled=value;}
    public boolean enabled(){return enabled;}
    /** Use only around a real function call on the real host tick. */
    public long begin(){return enabled?System.nanoTime():0;}
    public void end(Metric metric,long started){
        if(!enabled||started==0)return;
        long elapsed=System.nanoTime()-started;
        // Profiling must NEVER stop an otherwise functional host during a long
        // process pause or monotonic-clock wrap. Direct invalid test inputs
        // still fail fast in record(), but real timings are bounded safely.
        if(elapsed<0)return;
        record(metric,Math.min(MAX_NS,elapsed));
    }
    /** Direct samples exist only for deterministic unit tests and offline profilers. */
    public synchronized void record(Metric metric,long elapsedNs){
        if(metric==null||elapsedNs<0||elapsedNs>MAX_NS)
            throw new IllegalArgumentException("Invalid diagnostic duration");
        if(!enabled)return;
        recorded.get(metric).record(elapsedNs);
    }
    public synchronized Snapshot snapshot(){
        EnumMap<Metric,Percentiles> copy=new EnumMap<>(Metric.class);
        for(var e:recorded.entrySet())copy.put(e.getKey(),e.getValue().snapshot());
        return new Snapshot(enabled,WINDOW,copy);
    }
    /** Reset clears only diagnostic counters; does not modify the game. */
    public synchronized void clear(){
        for(Metric metric:Metric.values())recorded.put(metric,new Series());
    }
}
