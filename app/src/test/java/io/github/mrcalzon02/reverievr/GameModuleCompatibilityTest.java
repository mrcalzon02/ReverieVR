package io.github.mrcalzon02.reverievr;
import static org.junit.Assert.*;
import org.junit.Test;
public class GameModuleCompatibilityTest {
    private static GameModuleCatalog.Release fixture(){
        return new GameModuleCatalog.Release(
            "between-deliveries-red-ledger","0.2.0","content",
            "https://github.com/mrcalzon02/ReverieVR/releases/download/module-red-ledger-v0.2.0/red-ledger.rvpack",
            "012345678901234567890123456789012345678901234567890123456789abcd",
            "","testing",2,1024,"0.1.0",26,1,1,2,
            new String[]{"armeabi-v7a","arm64-v8a"});
    }
    @Test public void acceptsCompatibleGalaxyS9(){
        assertTrue(GameModuleCatalog.compatibleWith(fixture(),"0.2.0",26,1,2,
            new String[]{"arm64-v8a","armeabi-v7a"}));
    }
    @Test public void refusesNewerHubAndUnsupportedHardware(){
        GameModuleCatalog.Release r=fixture();
        assertFalse(GameModuleCatalog.compatibleWith(r,"0.0.9",26,1,2,
            new String[]{"arm64-v8a"}));
        assertFalse(GameModuleCatalog.compatibleWith(r,"0.2.0",25,1,2,
            new String[]{"arm64-v8a"}));
        assertFalse(GameModuleCatalog.compatibleWith(r,"0.2.0",26,2,2,
            new String[]{"arm64-v8a"}));
        assertFalse(GameModuleCatalog.compatibleWith(r,"0.2.0",26,1,1,
            new String[]{"arm64-v8a"}));
        assertFalse(GameModuleCatalog.compatibleWith(r,"0.2.0",26,1,2,
            new String[]{"x86"}));
    }
}
