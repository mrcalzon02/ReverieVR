package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import static org.junit.Assert.*;

public final class RvPackManifestTest {
    private static RvPackManifest parse(String json)throws IOException{
        return RvPackManifest.parse(json.getBytes(StandardCharsets.UTF_8));
    }
    @Test public void acceptsOwnedRedLedgerPack()throws Exception{
        RvPackManifest m=parse("{\"schema\":1,\"id\":\"between-deliveries-red-ledger\","+
            "\"kind\":\"content\",\"version\":\"0.1.0\",\"revision\":2,"+
            "\"files\":[{\"path\":\"assets/native/red-ledger/beckenburg/wall.rlprop\","+
            "\"bytes\":1536,\"sha256\":\"1234\"}]}");
        assertEquals("between-deliveries-red-ledger",m.id);
        assertEquals("0.1.0",m.version);
        assertEquals(2,m.revision);
    }
    @Test public void rejectsSpoofedNestedIdentityAndDuplicateKeys()throws Exception{
        reject("{\"metadata\":{\"id\":\"between-deliveries-red-ledger\"},"+
            "\"schema\":1,\"kind\":\"content\",\"revision\":1,\"version\":\"0.1.0\"}");
        reject("{\"schema\":1,\"kind\":\"content\",\"id\":\"omega-front\","+
            "\"id\":\"between-deliveries-red-ledger\","+
            "\"version\":\"0.1.0\",\"revision\":1}");
    }
    @Test public void rejectsInvalidRevisionAndMalformedJson()throws Exception {
        reject("{\"schema\":1,\"kind\":\"content\",\"id\":\"omega-front\","+
            "\"version\":\"0.1.0\",\"revision\":0}");
        reject("{\"schema\":1,\"kind\":\"content\",\"id\":\"omega-front\","+
            "\"version\":\"0.1.0\",\"revision\":-1}");
        reject("{\"schema\":1,\"kind\":\"content\",\"id\":\"omega-front\","+
            "\"version\":\"0.1.0\",\"revision\":1} trailing");
        reject("{\"schema\":1,\"kind\":\"content\",\"id\":\"omega-front\","+
            "\"version\":\"0.1.0\",\"revision\":1,\"data\":[{\"bad\":\"unterminated\"}");
    }
    @Test public void acceptsBoundedBulkCc0ContentButRejectsOversizedManifest()
        throws Exception {
        // The real Omega Front pack now carries 141 distinct files. Reject
        // anything beyond the shared 64-KiB bound without rejecting ordinary
        // licensed 2D tile libraries or bypassing strict JSON parsing.
        StringBuilder json=new StringBuilder("{\"schema\":1,\"kind\":\"content\",")
            .append("\"id\":\"omega-front\",\"version\":\"0.1.0\",")
            .append("\"revision\":1,\"files\":[");
        for(int i=0;i<170;i++){
            if(i>0)json.append(',');
            json.append("{\"path\":\"assets/native/omega-front/rpg_cc0/tile_")
                .append(i).append(".png\",\"bytes\":256,\"sha256\":\"")
                .append("a".repeat(64)).append("\"}");
        }
        json.append("]}");
        byte[] real=json.toString().getBytes(StandardCharsets.UTF_8);
        assertTrue(real.length>16384 && real.length<64*1024);
        assertEquals("omega-front",RvPackManifest.parse(real).id);
        byte[] oversized=new byte[64*1024+1];
        try{RvPackManifest.parse(oversized);
            fail("Oversized module.json accepted");}
        catch(IOException expected){assertNotNull(expected.getMessage());}
    }
    private static void reject(String json)throws Exception{
        try{parse(json);fail("Untrusted content manifest accepted");}
        catch(IOException expected){assertNotNull(expected.getMessage());}
    }
}