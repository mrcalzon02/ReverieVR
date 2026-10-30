package dev.mrcalzon.lighting;

import java.util.Arrays;

/**
 * Bounded, local CPU frame diagnostics. No allocations, logging, locks,
 * file I/O or GPU synchronization on the per-frame recording path.
 * Snapshot sorting is performed only on explicit diagnostic reads.
 */
public final class LightingFrameProfiler {
    public record Snapshot(int samples, double frameP50Ms, double frameP95Ms,
        double frameP99Ms, double sceneP95Ms, double rasterP95Ms,
        double psxPostP95Ms, double effectsPostP95Ms,
        double postTotalP95Ms, long peakTriangles, long peakShadowRays,
        long peakInvalidatedCells, int gpuReadbackMisses) {}

    private final long[] frame, scene, raster, psx, effects, post;
    private final long[] triangles, rays, invalidations;
    private final boolean[] misses;
    private int position, count;

    public LightingFrameProfiler(int window){
        if(window<4||window>4096)throw new IllegalArgumentException("invalid window");
        frame=new long[window];scene=new long[window];
        raster=new long[window];psx=new long[window];
        effects=new long[window];post=new long[window];
        triangles=new long[window];rays=new long[window];
        invalidations=new long[window];misses=new boolean[window];
    }
    public void record(long frameNs,long sceneNs,long rasterNs,
                       long psxNs,long effectsNs,long trianglesSubmitted,
                       long shadowRays,long invalidatedCells,
                       boolean gpuReadbackMissing){
        int i=position;
        frame[i]=Math.max(0,frameNs);
        scene[i]=Math.max(0,sceneNs);
        raster[i]=Math.max(0,rasterNs);
        psx[i]=Math.max(0,psxNs);
        effects[i]=Math.max(0,effectsNs);
        post[i]=Math.max(0,psxNs)+Math.max(0,effectsNs);
        triangles[i]=Math.max(0,trianglesSubmitted);
        rays[i]=Math.max(0,shadowRays);
        invalidations[i]=Math.max(0,invalidatedCells);
        misses[i]=gpuReadbackMissing;
        position=(i+1)%frame.length;
        if(count<frame.length)count++;
    }
    private double percentile(long[] source,double fraction){
        if(count==0)return 0;
        long[] sorted=Arrays.copyOf(source,count);
        Arrays.sort(sorted);
        int index=Math.max(0,(int)Math.ceil(count*fraction)-1);
        return sorted[Math.min(index,count-1)]/1_000_000.0;
    }
    private long peak(long[] source){
        long value=0;
        for(int i=0;i<count;i++)value=Math.max(value,source[i]);
        return value;
    }
    public Snapshot snapshot(){
        int missing=0;
        for(int i=0;i<count;i++)if(misses[i])missing++;
        return new Snapshot(count,
            percentile(frame,.5),percentile(frame,.95),percentile(frame,.99),
            percentile(scene,.95),percentile(raster,.95),
            percentile(psx,.95),percentile(effects,.95),
            percentile(post,.95),peak(triangles),peak(rays),
            peak(invalidations),missing);
    }
}
