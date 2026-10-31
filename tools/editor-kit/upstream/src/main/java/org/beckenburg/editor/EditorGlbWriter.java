package org.beckenburg.editor;

import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import javax.imageio.ImageIO;

/**
 * Game-compatible glTF 2.0 binary export for imported OBJ/STL geometry.
 * A small explicit fallback texture is used until source material binding is supported.
 * Geometry/UV source survives untouched in the imported original alongside this derivative.
 */
public final class EditorGlbWriter {
    private EditorGlbWriter(){}
    public static byte[] encode(EditorMeshPipeline.Mesh mesh)throws IOException {
        int count=mesh.triangles().size()*3;
        if(count<3||count>48000)throw new IOException(
            "Export needs 1..16000 triangles per model under existing 4 MiB GLB budget; reduce or split first");
        BufferedImage checker=new BufferedImage(16,16,BufferedImage.TYPE_INT_ARGB);
        for(int y=0;y<16;y++)for(int x=0;x<16;x++)
            checker.setRGB(x,y,(((x/4+y/4)&1)==0?0xff969c9d:0xff606d74));
        ByteArrayOutputStream image=new ByteArrayOutputStream();
        if(!ImageIO.write(checker,"png",image))throw new IOException("PNG image encoder unavailable");
        byte[] png=image.toByteArray();
        int posBytes=count*12,uvBytes=count*8,indexBytes=count*2;
        int imageStart=pad4(posBytes+uvBytes+indexBytes);
        int binSize=pad4(imageStart+png.length);
        ByteBuffer binary=ByteBuffer.allocate(binSize).order(ByteOrder.LITTLE_ENDIAN);
        for(var t:mesh.triangles())for(var v:new EditorMeshPipeline.Vertex[]{t.a(),t.b(),t.c()}){
            binary.putFloat((float)v.x());binary.putFloat((float)v.y());binary.putFloat((float)v.z());
        }
        for(var t:mesh.triangles())for(var v:new EditorMeshPipeline.Vertex[]{t.a(),t.b(),t.c()}){
            binary.putFloat((float)v.u());binary.putFloat((float)v.v());
        }
        for(int i=0;i<count;i++)binary.putShort((short)i);
        binary.position(imageStart);binary.put(png);
        String json="{\"asset\":{\"version\":\"2.0\",\"generator\":\"Beckenburg EditorAssetPipeline\"},"+
            "\"scene\":0,\"scenes\":[{\"nodes\":[0]}],\"nodes\":[{\"name\":\"Imported Model\",\"mesh\":0}],"+
            "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"TEXCOORD_0\":1},"+
            "\"indices\":2,\"material\":0,\"mode\":4}]}],"+
            "\"buffers\":[{\"byteLength\":"+binSize+"}],"+
            "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":"+posBytes+"},"+
            "{\"buffer\":0,\"byteOffset\":"+posBytes+",\"byteLength\":"+uvBytes+"},"+
            "{\"buffer\":0,\"byteOffset\":"+(posBytes+uvBytes)+",\"byteLength\":"+indexBytes+"},"+
            "{\"buffer\":0,\"byteOffset\":"+imageStart+",\"byteLength\":"+png.length+"}],"+
            "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":"+count+",\"type\":\"VEC3\"},"+
            "{\"bufferView\":1,\"componentType\":5126,\"count\":"+count+",\"type\":\"VEC2\"},"+
            "{\"bufferView\":2,\"componentType\":5123,\"count\":"+count+",\"type\":\"SCALAR\"}],"+
            "\"images\":[{\"bufferView\":3,\"mimeType\":\"image/png\"}],"+
            "\"textures\":[{\"source\":0}],"+
            "\"materials\":[{\"name\":\"PSX preview fallback atlas (replace with licensed art)\","+
            "\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0}}}]}";
        byte[] jsonRaw=json.getBytes(StandardCharsets.UTF_8);
        int jsonSize=pad4(jsonRaw.length),size=12+8+jsonSize+8+binSize;
        if(size>4_000_000)throw new IOException("Export exceeds the active 4 MiB game loader limit");
        ByteBuffer glb=ByteBuffer.allocate(size).order(ByteOrder.LITTLE_ENDIAN);
        glb.putInt(0x46546c67).putInt(2).putInt(size);
        glb.putInt(jsonSize).putInt(0x4e4f534a).put(jsonRaw);
        for(int i=jsonRaw.length;i<jsonSize;i++)glb.put((byte)' ');
        glb.putInt(binSize).putInt(0x004e4942).put(binary.array());
        return glb.array();
    }
    private static int pad4(int value){return (value+3)&~3;}
}
