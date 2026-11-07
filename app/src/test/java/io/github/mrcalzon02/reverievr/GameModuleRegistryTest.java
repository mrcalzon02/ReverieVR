package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;
import java.util.HashSet;

public class GameModuleRegistryTest {
    @Test public void knownModuleIdentitiesAreUnique(){
        HashSet<String> ids=new HashSet<>(),packages=new HashSet<>();
        for(GameModuleRegistry.Entry entry:GameModuleRegistry.GAMES){
            assertSame(entry,GameModuleRegistry.find(entry.id));
            assertEquals(entry.contentNamespace,RvPackInstaller.namespace(entry.id));
            assertTrue(ids.add(entry.id));
            assertTrue(packages.add(entry.companionPackage));
        }
        // Guess the Door is a registered development module in addition to
        // the previous fifteen; Sitka remains an inspectable project only.
        assertEquals(16,ids.size());
        assertNotNull(GameModuleRegistry.find("guess-the-door"));
        assertNull(GameModuleRegistry.find("sitka"));
        // The game must appear in the normal VR Games shelf and pass
        // NativeModuleRuntime.start without a Development logging gate.
        assertFalse(GameModuleRegistry.find("omega-front").developmentOnly);
        assertNotNull(GameModuleRegistry.find("procedural-test-chamber"));
        assertNull(GameModuleRegistry.find("classic-hypothetical"));
        assertNull(RvPackInstaller.namespace("classic-hypothetical"));
        assertNull(GameModuleRegistry.find("../red-ledger"));
    }
}
