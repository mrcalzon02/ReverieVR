package io.github.mrcalzon02.reverievr;

import java.io.File;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.io.DataInputStream;
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
        if(name.startsWith("assets/native/red-ledger/models/owned_")&&
           name.endsWith(".fbx"))verifyOwnedBarFbx(file);
    }
    private static void verifyOwnedBarFbx(File file)throws IOException {
        if(file.length()<100L||file.length()>100000L)
            throw new IOException("Owned Red Ledger FBX exceeds scoped size");
        byte[] header=new byte[27];
        try(DataInputStream in=new DataInputStream(
                new BufferedInputStream(new FileInputStream(file)))) {
            in.readFully(header);
        }
        byte[] magic="Kaydara FBX Binary  ".getBytes(
            java.nio.charset.StandardCharsets.US_ASCII);
        for(int i=0;i<magic.length;i++)
            if(header[i]!=magic[i])
                throw new IOException("Invalid owner model FBX signature");
        int version=ByteBuffer.wrap(header,23,4)
            .order(ByteOrder.LITTLE_ENDIAN).getInt();
        if(version!=7400)
            throw new IOException("Unsupported owner model FBX version");
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
        // Read and validate the same open file stream. No skipNBytes:
        // that newer JDK convenience method is not provided by Android 8.
        try(DataInputStream in=new DataInputStream(new BufferedInputStream(new FileInputStream(file)))){
            in.readFully(header);
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
            // Match the C++ native renderer: XYZ, UV and RGB are the first
            // eight floats; final two floats are currently unused.
            byte[] vertex=new byte[VERTEX_STRIDE];
            ByteBuffer values=ByteBuffer.wrap(vertex).order(ByteOrder.LITTLE_ENDIAN);
            for(int n=0;n<vertices;n++){
                in.readFully(vertex);
                for(int axis=0;axis<8;axis++){
                    float value=values.getFloat(axis*4);
                    if(!Float.isFinite(value))
                        throw new IOException("Nonfinite Beckenburg mesh vertex");
                    if(axis>=3 && (value<0.0f || value>1.0f))
                        throw new IOException("Invalid Beckenburg mesh UV/color");
                }
            }
        }
    }
}
