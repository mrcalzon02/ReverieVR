package org.beckenburg.editor.ai;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Objects;

/** Machine-readable aggregate-only profiling report. No actors or player fields. */
public final class NpcAiTelemetryReport {
    private NpcAiTelemetryReport(){}
    public static String csv(NpcAiTelemetry.Snapshot snapshot){
        Objects.requireNonNull(snapshot);
        StringBuilder out=new StringBuilder(
            "metric,enabled,total_calls,window_size,min_ns,p50_ns,p95_ns,p99_ns,max_ns,mean_ns\n");
        for(NpcAiTelemetry.Metric metric:NpcAiTelemetry.Metric.values()){
            var t=snapshot.timings().get(metric);
            out.append(metric).append(',').append(snapshot.enabled()).append(',')
                .append(t.calls()).append(',').append(snapshot.windowSize()).append(',')
                .append(t.minimumNs()).append(',').append(t.p50Ns()).append(',')
                .append(t.p95Ns()).append(',').append(t.p99Ns()).append(',')
                .append(t.maximumNs()).append(',').append(t.meanNs()).append('\n');
        }
        return out.toString();
    }
    public static void save(Path target,NpcAiTelemetry.Snapshot snapshot)throws IOException{
        Path dir=target.toAbsolutePath().getParent();
        Files.createDirectories(dir);
        Path temp=Files.createTempFile(dir,"ai-metrics-", ".pending");
        try{
            Files.writeString(temp,csv(snapshot),StandardCharsets.UTF_8);
            try{Files.move(temp,target,StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);}
            catch(java.nio.file.AtomicMoveNotSupportedException ex){
                Files.move(temp,target,StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(temp);}
    }
}
