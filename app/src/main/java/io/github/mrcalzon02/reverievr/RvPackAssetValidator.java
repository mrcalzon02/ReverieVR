package io.github.mrcalzon02.reverievr;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * On-disk validation of small game data formats before a new content revision
 * becomes active. Does not parse executable code or allocate pixel buffers.
 */
final class RvPackAssetValidator {
    private static final long ATLAS_BYTES=256L*256L*2L;
    private static final int VERTEX_STRIDE=10*4;
    private static final int MAX_VERTICES=50000;
    private RvPackAssetValidator(){}

    static void verify(String name,File file)throws IOException {
        if(name==null||file==null||!file.isFile())
            throw new IOException("Missing content asset");
        if(name.endsWith(".rlprop"))verifyRoomMesh(file);
        if(name.endsWith(".bwm"))verifyBreakwaterMesh(file);
    }
    private static void verifyBreakwaterMesh(File file)throws IOException {
        final long limit=8L+2048L*27L;
        if(file.length()<35L||file.length()>limit)
            throw new IOException("Invalid BWM1 game mesh size");
        byte[] bytes=java.nio.file.Files.readAllBytes(file.toPath());
        ByteBuffer buffer=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        if(buffer.get()!=66||buffer.get()!=87||buffer.get()!=77||buffer.get()!=49)
            throw new IOException("Invalid BWM1 game mesh magic");
        int triangles=buffer.getInt();
        if(triangles<1||triangles>2048||bytes.length!=8L+(long)triangles*27L)
            throw new IOException("Invalid BWM1 triangle count or byte length");
        // Three authentic XYZ/color vertices per triangle, with signed
        // 16-bit coordinates. Match the C++ model parser exactly.
        for(int at=8;at<bytes.length;at+=9){
            for(int axis=0;axis<3;axis++){
                short value=buffer.getShort(at+axis*2);
                if(value<=-32000||value>=32000)
                    throw new IOException("Invalid BWM1 vertex coordinate");
            }
        }
    }
    private static void verifyRoomMesh(File file)throws IOException {
        byte[] header=new byte[16];
        if(file.length()<header.length+ATLAS_BYTES+VERTEX_STRIDE*3L)
            throw new IOException("Truncated Beckenburg room model");
        try(FileInputStream in=new FileInputStream(file)){
            int count=0;
            while(count<header.length){
                int got=in.read(header,count,header.length-count);
                if(got<0)throw new IOException("Truncated mesh header");
                count+=got;
            }
        }
        ByteBuffer b=ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);
        if(b.get()!=82||b.get()!=76||b.get()!=66||b.get()!=80)
            throw new IOException("Unexpected RLBP model magic");
        int version=b.getInt(),vertices=b.getInt(),atlasSide=b.getInt();
        if(version!=1||vertices<3||vertices%3!=0||
           vertices>MAX_VERTICES||atlasSide!=256)
            throw new IOException("Unsupported Beckenburg model layout");
        long expected=16L+(long)vertices*VERTEX_STRIDE+ATLAS_BYTES;
        if(file.length()!=expected)
            throw new IOException("Beckenburg model length mismatch");
    }
}
