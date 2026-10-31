package org.beckenburg.editor.ai;

public final class NpcAiTelemetryTest {
    private static void check(boolean yes,String why){if(!yes)throw new AssertionError(why);}
    public static void main(String[] args){
        var live=new NpcAiTelemetry();
        long ignored=live.begin();
        check(ignored==0,"Opt-in default is off");
        live.record(NpcAiTelemetry.Metric.DECISION,123);
        check(live.snapshot().timings().get(NpcAiTelemetry.Metric.DECISION).calls()==0,
            "No hidden records before opt-in");
        live.setEnabled(true);
        for(int i=1;i<=300;i++)live.record(NpcAiTelemetry.Metric.DECISION,i*1000L);
        var snap=live.snapshot();
        var p=snap.timings().get(NpcAiTelemetry.Metric.DECISION);
        check(p.calls()==300,"All calls counted");
        check(p.minimumNs()==45000&&p.maximumNs()==300000,"Ring holds 256 newest samples");
        check(p.p50Ns()>p.minimumNs()&&p.p95Ns()>=p.p50Ns()
              &&p.p99Ns()>=p.p95Ns(),"Correct ordered percentiles");
        check(snap.windowSize()==256&&snap.timings().size()==NpcAiTelemetry.Metric.values().length,
            "Every metric has bounded window");
        long start=live.begin();
        int n=0;for(int i=0;i<1000;i++)n+=i;
        live.end(NpcAiTelemetry.Metric.HEARING,start);
        check(live.snapshot().timings().get(NpcAiTelemetry.Metric.HEARING).calls()==1,
            "Actual measured timing recorded");
        String export=NpcAiTelemetryReport.csv(snap);
        check(export.startsWith("metric,enabled,total_calls")&&
            export.contains("DECISION,true,300,256,45000,"),
            "Aggregate report includes counts and ring metrics only");
        check(!export.contains("hunter")&&!export.contains("room")&&!export.contains("ghost"),
            "Report has no actor, location or player secrets");
        try{
            java.nio.file.Path temp=java.nio.file.Files.createTempDirectory("ai-telemetry-");
            try{
                java.nio.file.Path path=temp.resolve("profile.csv");
                NpcAiTelemetryReport.save(path,snap);
                check(java.nio.file.Files.readString(path).equals(export),
                    "CSV export atomically persists the aggregate only");
            }finally{
                try(var files=java.nio.file.Files.list(temp)){
                    files.forEach(x->{try{java.nio.file.Files.deleteIfExists(x);}catch(Exception ignored){}});
                }
                java.nio.file.Files.deleteIfExists(temp);
            }
        }catch(java.io.IOException err){throw new AssertionError("CSV persistence failed",err);}
        live.setEnabled(false);
        live.record(NpcAiTelemetry.Metric.DECISION,100);
        check(live.snapshot().timings().get(NpcAiTelemetry.Metric.DECISION).calls()==300,
            "Disabling never changes previous data");
        live.clear();
        check(live.snapshot().timings().get(NpcAiTelemetry.Metric.DECISION).calls()==0,
            "Clear only resets diagnostics");
        boolean refused=false;
        try{live.record(NpcAiTelemetry.Metric.ROUTE,-1);}
        catch(IllegalArgumentException expected){refused=true;}
        check(refused,"Invalid duration rejected even while disabled");
        System.out.println("PASS bounded per-host AI telemetry, percentile ring, opt-in and diagnostic reset");
    }
}
