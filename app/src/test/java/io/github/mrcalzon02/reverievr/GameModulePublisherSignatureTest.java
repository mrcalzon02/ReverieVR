package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.Signature;
import java.security.MessageDigest;
import java.security.spec.X509EncodedKeySpec;

/** Executable JVM crypto regression independent of Android's JSON mock. */
public final class GameModulePublisherSignatureTest {
    private static Path repoRoot() throws IOException {
        Path cwd=Paths.get(System.getProperty("user.dir")).toAbsolutePath();
        Path root=Files.isRegularFile(cwd.resolve("modules/catalog-v1.json"))
            ?cwd:cwd.getParent();
        if(root==null||
           !Files.isRegularFile(root.resolve("modules/catalog-v1.json")))
            throw new IOException("Signed module-catalog test fixture unavailable");
        return root;
    }
    @Test public void signedTestingFeedIsAuthenticAndTamperResistant()
        throws Exception {
        Path root=repoRoot();
        byte[] pub=Files.readAllBytes(root.resolve(
            "app/src/main/assets/native/modules/catalog-key.der"));
        byte[] sig=Files.readAllBytes(root.resolve("modules/catalog-v1.sig"));
        byte[] json=Files.readAllBytes(root.resolve("modules/catalog-v1.json"));
        byte[] publicDigest=MessageDigest.getInstance("SHA-256").digest(pub);
        StringBuilder fingerprint=new StringBuilder();
        for(byte value:publicDigest)fingerprint.append(String.format(
            java.util.Locale.ROOT,"%02x",value&0xff));
        assertEquals("2c51148234e9639f599acea22802cd20fed646fe43211e39e26b9e640b670a6d",
            fingerprint.toString());
        PublicKey key=KeyFactory.getInstance("EC").generatePublic(
            new X509EncodedKeySpec(pub));
        Signature verifier=Signature.getInstance("SHA256withECDSA");
        verifier.initVerify(key);
        verifier.update(json);
        assertTrue("Bundled catalog signature rejected",verifier.verify(sig));
        byte[] altered=json.clone();
        altered[0]^=1;
        verifier.initVerify(key);
        verifier.update(altered);
        assertFalse("Catalog tampering went undetected",verifier.verify(sig));
        assertTrue("Do not announce nonexistent module releases",
            new String(json,java.nio.charset.StandardCharsets.UTF_8)
                .contains("\"modules\":[]"));
    }
}
