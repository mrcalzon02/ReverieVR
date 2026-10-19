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
    @Test public void rejectsNonfiniteAndInvalidTextureCoordinates()throws Exception{
        File mesh=Files.createTempFile("mesh-invalid-floats",".rlprop").toFile();
        byte[] bytes=new byte[16+3*40+256*256*2];
        bytes[0]='R';bytes[1]='L';bytes[2]='B';bytes[3]='P';
        bytes[4]=1;bytes[8]=3;bytes[13]=1;
        java.nio.ByteBuffer values=java.nio.ByteBuffer.wrap(bytes)
            .order(java.nio.ByteOrder.LITTLE_ENDIAN);
        // XYZ UV RGB are the first eight floats of each RLBP vertex.
        values.putFloat(16,Float.NaN);
        Files.write(mesh.toPath(),bytes);
        try{
            RvPackAssetValidator.verify("beckenburg/room.rlprop",mesh);
            fail("Accepted a nonfinite position");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("Nonfinite"));}
        values.putFloat(16,0.0f);
        values.putFloat(16+3*4,1.25f);
        Files.write(mesh.toPath(),bytes);
        try{
            RvPackAssetValidator.verify("beckenburg/room.rlprop",mesh);
            fail("Accepted out of range UV");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("UV/color"));}
        values.putFloat(16+3*4,0.5f);
        values.putFloat(16+7*4,Float.POSITIVE_INFINITY);
        Files.write(mesh.toPath(),bytes);
        try{
            RvPackAssetValidator.verify("beckenburg/room.rlprop",mesh);
            fail("Accepted nonfinite RGB");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("Nonfinite"));}
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
    @Test public void validatesScopedOwnerBlenderModelAndRejectsVersion()
        throws Exception {
        File file=Files.createTempFile("owned-bar",".fbx").toFile();
        byte[] bytes=new byte[100];
        byte[] magic="Kaydara FBX Binary  ".getBytes(
            java.nio.charset.StandardCharsets.US_ASCII);
        System.arraycopy(magic,0,bytes,0,magic.length);
        bytes[23]=(byte)0xE8;bytes[24]=(byte)0x1C; // FBX version 7400
        Files.write(file.toPath(),bytes);
        RvPackAssetValidator.verify(
            "assets/native/red-ledger/models/owned_cigar.fbx",file);
        bytes[0]=0;Files.write(file.toPath(),bytes);
        try {
            RvPackAssetValidator.verify(
                "assets/native/red-ledger/models/owned_cigar.fbx",file);
            fail("Corrupt selected Blender FBX accepted");
        } catch(IOException expected){
            assertTrue(expected.getMessage().contains("signature"));
        }
    }
    @Test public void rejectsTruncatedAsset()throws Exception{
        File file=Files.createTempFile("empty",".rlprop").toFile();
        try{RvPackAssetValidator.verify("empty.rlprop",file);fail("Empty room mesh");}
        catch(IOException expected){assertTrue(expected.getMessage().contains("Truncated"));}
    }
}
