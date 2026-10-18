package io.github.mrcalzon02.reverievr.game.testchamber;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;

/**
 * Minimal read-only GLB 2.0 loader for Beckenburg PSX showcase meshes.
 * Supports the actual owned static meshes, embedded PNG base-color images,
 * triangle indices (u8/u16/u32) and float XYZ/UV accessors. Unsupported
 * formats are rejected; no JSON/executable code from a network is evaluated.
 */
final class TestChamberModels {
    static final String[] APPROVED={
        "crate.glb","chair.glb","shelf.glb","table.glb","door.glb"
    };
    static final int MAX_BYTES=4*1024*1024;
    static final int MAX_VERTICES=18000;
    private TestChamberModels(){}

    static final class Primitive {
        final int modelId, width, height;
        final float[] vertices; // XYZ, UV in tightly interleaved GLES2 layout
        final int[] argb; // decoded PNG, RGBA8888 conversion in native bridge
        Primitive(int id,float[] vertices,int width,int height,int[] argb){
            this.modelId=id;this.vertices=vertices;
            this.width=width;this.height=height;this.argb=argb;
        }
    }

    static List<Primitive> load(AssetManager assets)throws Exception{
        ArrayList<Primitive> result=new ArrayList<>();
        for(int model=0;model<APPROVED.length;model++){
            byte[] bytes=read(assets,"models/"+APPROVED[model]);
            ByteBuffer file=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
            if(bytes.length<28||file.getInt(0)!=0x46546c67||
               file.getInt(4)!=2||file.getInt(8)!=bytes.length)
                throw new IOException("Invalid GLB header "+APPROVED[model]);
            int jsonSize=file.getInt(12),jsonTag=file.getInt(16);
            if(jsonTag!=0x4e4f534a||jsonSize<2||
               jsonSize>bytes.length-28)
                throw new IOException("Unsupported GLB JSON chunk");
            String text=new String(bytes,20,jsonSize,
                java.nio.charset.StandardCharsets.UTF_8);
            JSONObject gltf=new JSONObject(text);
            int binHeader=20+jsonSize;
            if(file.getInt(binHeader+4)!=0x004e4942)
                throw new IOException("Missing GLB BIN chunk");
            int binSize=file.getInt(binHeader),binStart=binHeader+8;
            if(binSize<0||binStart+binSize>bytes.length)
                throw new IOException("Truncated GLB BIN chunk");
            JSONArray views=gltf.getJSONArray("bufferViews"),
                accessors=gltf.getJSONArray("accessors"),
                images=gltf.getJSONArray("images"),
                textures=gltf.getJSONArray("textures"),
                materials=gltf.getJSONArray("materials"),
                meshes=gltf.getJSONArray("meshes"),
                nodes=gltf.getJSONArray("nodes");
            if(meshes.length()>8||materials.length()>16)
                throw new IOException("Excessive static-mesh components");
            for(int mi=0;mi<meshes.length();mi++){
                // Blender's separate door handle is a child node with a
                // local translation. Flatten nodes before uploading static
                // world-space triangle arrays; preserve real model geometry.
                float[] offset=new float[3];
                for(int ni=0;ni<nodes.length();ni++){
                    JSONObject node=nodes.getJSONObject(ni);
                    if(node.optInt("mesh",-1)!=mi)continue;
                    JSONArray xyz=node.optJSONArray("translation");
                    if(xyz!=null){
                        if(xyz.length()!=3)throw new IOException("Malformed glTF node translation");
                        for(int axis=0;axis<3;axis++)
                            offset[axis]=(float)xyz.getDouble(axis);
                    }
                    if(node.has("rotation")||node.has("scale"))
                        throw new IOException("Unimplemented model rotation/scale; reject instead of deforming");
                }
                JSONArray pieces=meshes.getJSONObject(mi).getJSONArray("primitives");
                if(pieces.length()>16)throw new IOException("Excessive GLB primitives");
                for(int pi=0;pi<pieces.length();pi++){
                    JSONObject piece=pieces.getJSONObject(pi);
                    if(piece.optInt("mode",4)!=4)
                        throw new IOException("Unsupported non-triangle GLB mesh");
                    JSONObject attrs=piece.getJSONObject("attributes");
                    int pos=attrs.getInt("POSITION"),
                        uv=attrs.getInt("TEXCOORD_0"),
                        indices=piece.getInt("indices");
                    JSONObject positions=accessors.getJSONObject(pos);
                    JSONObject uvs=accessors.getJSONObject(uv);
                    JSONObject index=accessors.getJSONObject(indices);
                    if(positions.getInt("componentType")!=5126||
                       uvs.getInt("componentType")!=5126||
                       !"VEC3".equals(positions.getString("type"))||
                       !"VEC2".equals(uvs.getString("type"))||
                       !"SCALAR".equals(index.getString("type")))
                        throw new IOException("Unsupported GLB vertex components");
                    int count=positions.getInt("count"),tris=index.getInt("count");
                    if(count<=0||count>MAX_VERTICES||count!=uvs.getInt("count")||
                       tris<3||tris>MAX_VERTICES||tris%3!=0)
                        throw new IOException("Invalid GLB triangle limits");
                    float[] vertices=new float[tris*5];
                    for(int k=0;k<tris;k++){
                        long vi=readIndex(file,index,views,binStart,binSize,k);
                        if(vi<0||vi>=count)throw new IOException("Index outside GLB mesh");
                        int idx=(int)vi;
                        vertices[k*5]=offset[0]+readFloat(file,positions,views,binStart,binSize,
                            idx,0,3);
                        vertices[k*5+1]=offset[1]+readFloat(file,positions,views,binStart,binSize,
                            idx,1,3);
                        vertices[k*5+2]=offset[2]+readFloat(file,positions,views,binStart,binSize,
                            idx,2,3);
                        vertices[k*5+3]=readFloat(file,uvs,views,binStart,binSize,
                            idx,0,2);
                        vertices[k*5+4]=1.f-readFloat(file,uvs,views,binStart,binSize,
                            idx,1,2); // Android PNG top-left orientation
                        for(int j=0;j<5;j++)
                            if(!Float.isFinite(vertices[k*5+j]))
                                throw new IOException("Non-finite GLB vertex");
                    }
                    int material=piece.getInt("material");
                    int tx=materials.getJSONObject(material)
                        .getJSONObject("pbrMetallicRoughness")
                        .getJSONObject("baseColorTexture").getInt("index");
                    int imageId=textures.getJSONObject(tx).getInt("source");
                    JSONObject image=images.getJSONObject(imageId);
                    if(!"image/png".equals(image.getString("mimeType")))
                        throw new IOException("Only owner-packaged PNG atlas supported");
                    JSONObject imageView=views.getJSONObject(image.getInt("bufferView"));
                    int offset=imageView.optInt("byteOffset",0),
                        length=imageView.getInt("byteLength");
                    if(offset<0||length<=0||length>2*1024*1024||
                       offset+length>binSize)
                        throw new IOException("Invalid embedded PNG bounds");
                    Bitmap bitmap=BitmapFactory.decodeByteArray(bytes,
                        binStart+offset,length);
                    if(bitmap==null||bitmap.getWidth()<1||bitmap.getHeight()<1||
                       bitmap.getWidth()>1024||bitmap.getHeight()>1024)
                        throw new IOException("Embedded PNG atlas decoder failure");
                    int w=bitmap.getWidth(),h=bitmap.getHeight();
                    int[] colors=new int[w*h];
                    bitmap.getPixels(colors,0,w,0,0,w,h);
                    bitmap.recycle();
                    result.add(new Primitive(model,vertices,w,h,colors));
                }
            }
        }
        if(result.size()<APPROVED.length)
            throw new IOException("One or more approved showcase models missing");
        return result;
    }

    private static byte[] read(AssetManager assets,String path)throws IOException{
        try(InputStream in=assets.open(path);
            ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] buf=new byte[16384];int total=0,n;
            while((n=in.read(buf))!=-1){
                total+=n;
                if(total>MAX_BYTES)throw new IOException("Oversized GLB "+path);
                out.write(buf,0,n);
            }
            return out.toByteArray();
        }
    }
    private static int viewStart(JSONObject acc,JSONArray views,int binStart,
                                 int binSize,int element,int component,
                                 int componentBytes,int vectorSize)throws Exception{
        JSONObject view=views.getJSONObject(acc.getInt("bufferView"));
        int offset=view.optInt("byteOffset",0),length=view.getInt("byteLength"),
            local=acc.optInt("byteOffset",0);
        int stride=view.optInt("byteStride",componentBytes*vectorSize);
        long address=(long)offset+local+(long)element*stride+
            (long)component*componentBytes;
        if(offset<0||length<0||local<0||stride<componentBytes*vectorSize ||
           (long)offset+length>binSize||address<offset||
           address+componentBytes>(long)offset+length)
            throw new IOException("GLB accessor bounds exceeded");
        return binStart+(int)address;
    }
    private static float readFloat(ByteBuffer src,JSONObject acc,JSONArray views,
                                   int start,int size,int idx,int component,
                                   int vec)throws Exception{
        int at=viewStart(acc,views,start,size,idx,component,4,vec);
        return src.getFloat(at);
    }
    private static long readIndex(ByteBuffer src,JSONObject acc,JSONArray views,
                                  int start,int size,int idx)throws Exception{
        int type=acc.getInt("componentType"),
            bytes=type==5121?1:type==5123?2:type==5125?4:0;
        if(bytes==0)throw new IOException("Unsupported GLB index component");
        int at=viewStart(acc,views,start,size,idx,0,bytes,1);
        if(bytes==1)return src.get(at)&255L;
        if(bytes==2)return src.getShort(at)&65535L;
        return src.getInt(at)&0xffffffffL;
    }
}