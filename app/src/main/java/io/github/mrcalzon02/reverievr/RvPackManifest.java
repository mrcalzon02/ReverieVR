package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * Strict, dependency-free JSON metadata reader for content archives.
 * Verifies top-level game identity and revision without accepting lookalikes
 * placed inside nested objects. This is NOT an arbitrary JSON data engine.
 */
final class RvPackManifest {
    final String id, version;
    final long revision;
    private RvPackManifest(String id, String version, long revision) {
        this.id=id; this.version=version; this.revision=revision;
    }
    static RvPackManifest parse(byte[] input) throws IOException {
        if(input==null||input.length==0||input.length>16384)
            throw new IOException("Invalid module manifest length");
        final String json;
        try {
            json=StandardCharsets.UTF_8.newDecoder().decode(
                ByteBuffer.wrap(input)).toString();
        }catch(CharacterCodingException e){
            throw new IOException("Invalid module manifest UTF-8",e);
        }
        Reader r=new Reader(json);
        Map<String,String> metadata=r.readObject(0);
        r.whitespace();
        if(r.at!=json.length())
            throw new IOException("Unexpected trailing manifest bytes");
        if(!"1".equals(metadata.get("schema")) ||
           !"content".equals(metadata.get("kind")))
            throw new IOException("Unsupported module content schema");
        String id=metadata.get("id"),version=metadata.get("version"),
               rawRev=metadata.get("revision");
        if(RvPackInstaller.namespace(id)==null ||
           rawRev==null || !rawRev.matches("[1-9][0-9]{0,9}"))
            throw new IOException("Invalid module identity or revision");
        long revision=Long.parseLong(rawRev);
        if(revision>Integer.MAX_VALUE)
            throw new IOException("Module revision overflow");
        if(version==null ||
           !version.matches("[0-9]+\\.[0-9]+\\.[0-9]+(-[a-z0-9.-]+)?"))
            throw new IOException("Invalid content semantic version");
        return new RvPackManifest(id,version,revision);
    }
    private static final class Reader {
        final String json;
        int at;
        Reader(String source){json=source;}
        void whitespace(){
            while(at<json.length()){
                char c=json.charAt(at);
                if(c==' '||c=='\r'||c=='\n'||c=='\t')++at;
                else break;
            }
        }
        char peek()throws IOException {
            whitespace();
            if(at>=json.length())throw new IOException("Truncated JSON");
            return json.charAt(at);
        }
        void expect(char c)throws IOException {
            if(peek()!=c)throw new IOException("Invalid module JSON structure");
            ++at;
        }
        String quoted()throws IOException{
            expect('"');
            StringBuilder result=new StringBuilder();
            boolean terminated=false;
            while(at<json.length()){
                char c=json.charAt(at++);
                if(c=='"'){terminated=true;break;}
                if(c=='\\'){
                    if(at>=json.length())throw new IOException("Truncated JSON escape");
                    char e=json.charAt(at++);
                    switch(e){
                        case '"':case '\\':case '/':result.append(e);break;
                        case 'b':result.append('\b');break;
                        case 'f':result.append('\f');break;
                        case 'n':result.append('\n');break;
                        case 'r':result.append('\r');break;
                        case 't':result.append('\t');break;
                        case 'u':
                            if(at+4>json.length())
                                throw new IOException("Truncated JSON Unicode");
                            try{result.append((char)Integer.parseInt(
                                json.substring(at,at+4),16));}
                            catch(NumberFormatException ex){
                                throw new IOException("Invalid JSON Unicode",ex);
                            }
                            at+=4;break;
                        default:throw new IOException("Invalid JSON escape");
                    }
                }else {
                    if(c<32)throw new IOException("Invalid JSON control character");
                    result.append(c);
                }
            }
            if(!terminated)throw new IOException("Unterminated JSON string");
            return result.toString();
        }
        String value(int depth)throws IOException{
            if(depth>12)throw new IOException("Module JSON too deeply nested");
            char c=peek();
            if(c=='"')return quoted();
            if(c=='{'){readObject(depth+1);return null;}
            if(c=='['){readArray(depth+1);return null;}
            int start=at;
            while(at<json.length()){
                c=json.charAt(at);
                if(c==','||c=='}'||c==']'||Character.isWhitespace(c))break;
                ++at;
            }
            if(start==at)throw new IOException("Missing JSON value");
            String atom=json.substring(start,at);
            if(!atom.matches("true|false|null|-?(?:0|[1-9][0-9]*)(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?"))
                throw new IOException("Invalid JSON atom");
            return atom;
        }
        Map<String,String> readObject(int depth)throws IOException{
            if(depth>12)throw new IOException("Module JSON object too deeply nested");
            expect('{');
            Map<String,String> result=new HashMap<>();
            Set<String> keys=new HashSet<>();
            if(peek()=='}'){++at;return result;}
            while(true){
                if(peek()!='"')throw new IOException("Expected JSON object key");
                String key=quoted();
                if(!keys.add(key))throw new IOException("Duplicate module JSON field");
                expect(':');
                String scalar=value(depth+1);
                result.put(key,scalar);
                char next=peek();
                if(next=='}'){++at;return result;}
                expect(',');
            }
        }
        void readArray(int depth)throws IOException{
            if(depth>12)throw new IOException("Module JSON array too deeply nested");
            expect('[');
            if(peek()==']'){++at;return;}
            while(true){
                value(depth+1);
                char next=peek();
                if(next==']'){++at;return;}
                expect(',');
            }
        }
    }
}
