package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.URL;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public final class LaptopLinkDownloadServerTest {
    private static LaptopLinkBundle.Assets sampleAssets() {
        return path -> new ByteArrayInputStream(
            ("trusted bundled asset: " + path).getBytes("UTF-8"));
    }

    @Test public void oneAuthorizedDownloadThenListenerStops() throws Exception {
        CountDownLatch finished = new CountDownLatch(1);
        try (LaptopLinkDownloadServer server = new LaptopLinkDownloadServer(
                sampleAssets(), InetAddress.getLoopbackAddress(),
                delivered -> finished.countDown())) {
            String url = server.url("127.0.0.1");
            assertTrue(url.contains("/reverie-link/"));
            assertFalse(url.contains("password"));
            HttpURLConnection connection = (HttpURLConnection)new URL(url).openConnection();
            connection.setConnectTimeout(1500);
            connection.setReadTimeout(3000);
            assertEquals(200,connection.getResponseCode());
            assertEquals("application/zip",connection.getContentType());
            assertTrue(connection.getHeaderField("Content-Disposition")
                .contains(LaptopLinkBundle.EXPORT_NAME));
            byte[] body;
            try (InputStream in=connection.getInputStream()) {
                body=in.readAllBytes();
            }
            connection.disconnect();
            assertArrayEquals(MessageDigest.getInstance("SHA-256").digest(body),
                hexToBytes(server.sha256()));
            assertTrue(body.length>100);
            assertTrue(finished.await(3,TimeUnit.SECONDS));
            assertTrue(server.wasDelivered());
            assertFalse(server.isRunning());
        }
    }

    @Test public void guessedUrlCannotStealBundleAndDoesNotConsumeSession()
            throws Exception {
        try(LaptopLinkDownloadServer server=new LaptopLinkDownloadServer(
                sampleAssets(),InetAddress.getLoopbackAddress(),null)) {
            String url=server.url("127.0.0.1");
            URL guessed=new URL(url.substring(0,url.lastIndexOf('/')+1)+"wrong");
            HttpURLConnection wrong=(HttpURLConnection)guessed.openConnection();
            wrong.setReadTimeout(2000);
            assertEquals(404,wrong.getResponseCode());
            wrong.disconnect();
            assertTrue(server.isRunning());
            HttpURLConnection good=(HttpURLConnection)new URL(url).openConnection();
            good.setReadTimeout(2000);
            assertEquals(200,good.getResponseCode());
            try(InputStream in=good.getInputStream()){
                assertTrue(in.readAllBytes().length>50);
            }
            good.disconnect();
        }
    }

    @Test public void explicitStopDoesNotLeakEndpoint() throws Exception {
        LaptopLinkDownloadServer server=new LaptopLinkDownloadServer(
            sampleAssets(),InetAddress.getLoopbackAddress(),null);
        server.close();
        assertFalse(server.isRunning());
    }

    private static byte[] hexToBytes(String hex) {
        byte[] result=new byte[hex.length()/2];
        for(int i=0;i<result.length;i++)
            result[i]=(byte)Integer.parseInt(hex.substring(i*2,i*2+2),16);
        return result;
    }
}
