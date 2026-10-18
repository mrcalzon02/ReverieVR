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
