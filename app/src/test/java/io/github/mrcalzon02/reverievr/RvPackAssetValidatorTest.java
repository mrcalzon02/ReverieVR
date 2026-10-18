package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;
import java.io.*;
import java.nio.file.Files;

public final class RvPackAssetValidatorTest {
    @Test public void validatesMinimalRlbpModel()throws Exception{
        File model=Files.createTempFile("room-model",".rlprop").toFile();
        byte[] data=new byte[16+3*40+256*256*2];
        data[0]='R';data[1]='L';data[2]='B';data[3]='P';
        data[4]=1;data[8]=3;data[13]=1;
        try(FileOutputStream out=new FileOutputStream(model)){out.write(data);}
        RvPackAssetValidator.verify("room.rlprop",model);
        data[4]=2;
        try(FileOutputStream out=new FileOutputStream(model)){out.write(data);}
        try{
            RvPackAssetValidator.verify("room.rlprop",model);
            fail("Unsupported model version accepted");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("layout"));}
    }
    @Test public void validatesAuthenticBreakwaterMeshAndRejectsCorruption()
        throws Exception{
        File file=Files.createTempFile("beckenburg-game",".bwm").toFile();
        byte[] bytes=new byte[8+27];
        bytes[0]='B';bytes[1]='W';bytes[2]='M';bytes[3]='1';
        bytes[4]=1; // one BWM1 triangle
        try(FileOutputStream out=new FileOutputStream(file)){out.write(bytes);}
        RvPackAssetValidator.verify("concrete_block.bwm",file);
        bytes[4]=2;
        try(FileOutputStream out=new FileOutputStream(file)){out.write(bytes);}
        try{
            RvPackAssetValidator.verify("concrete_block.bwm",file);
            fail("Truncated BWM1 triangle accepted");
        }catch(IOException expected){
            assertTrue(expected.getMessage().contains("triangle count"));
        }
        bytes[4]=1;
        bytes[8]=0;bytes[9]=125; // little-endian 32000, beyond admissible XYZ
        try(FileOutputStream out=new FileOutputStream(file)){out.write(bytes);}
        try{
            RvPackAssetValidator.verify("concrete_block.bwm",file);
            fail("Out-of-range BWM1 coordinate accepted");
        }catch(IOException expected){
            assertTrue(expected.getMessage().contains("coordinate"));
        }
    }
    @Test public void rejectsTruncatedAsset()throws Exception{
        File file=Files.createTempFile("empty",".rlprop").toFile();
        try{RvPackAssetValidator.verify("empty.rlprop",file);fail("Empty room mesh");}
        catch(IOException expected){assertTrue(expected.getMessage().contains("Truncated"));}
    }
}
