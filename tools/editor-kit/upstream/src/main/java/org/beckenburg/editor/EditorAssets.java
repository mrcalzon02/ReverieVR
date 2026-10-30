package org.beckenburg.editor;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;

/** Project-agnostic virtual asset paths, with explicit providers. */
public final class EditorAssets {
    private EditorAssets(){}
    @FunctionalInterface public interface Provider {
        InputStream open(String normalizedRelativePath) throws IOException;
    }
    public static final class Registry {
        private final Map<String,Provider> providers=new LinkedHashMap<>();
        public Registry register(String scheme,Provider provider){
            if(scheme==null||!scheme.matches("[a-z][a-z0-9+.-]*")
                ||provider==null)throw new IllegalArgumentException("Invalid provider");
            providers.put(scheme,provider);return this;
        }
        public InputStream open(String uri)throws IOException{
            validateUri(uri);
            int split=uri.indexOf("://");
            Provider provider=providers.get(uri.substring(0,split));
            if(provider==null)throw new IOException("Unknown editor asset scheme: "+uri);
            return provider.open(uri.substring(split+3));
        }
    }
    public static void validateUri(String uri){
        if(uri==null||!uri.matches("[a-z][a-z0-9+.-]*://[A-Za-z0-9_./-]+")
            ||uri.contains("..")||uri.indexOf("//",uri.indexOf("://")+3)>=0
            ||uri.substring(uri.indexOf("://")+3).startsWith("/")
            ||uri.endsWith("/"))throw new IllegalArgumentException("Unsafe asset URI: "+uri);
    }
    public static Provider filesystem(Path root){
        Path safeRoot=root.toAbsolutePath().normalize();
        return path->{
            Path target=safeRoot.resolve(path).normalize();
            if(!target.startsWith(safeRoot))throw new IOException("Asset escapes root");
            return Files.newInputStream(target);
        };
    }
    public static Provider classpath(ClassLoader loader,String prefix){
        if(loader==null||prefix==null||prefix.startsWith("/")||prefix.contains(".."))
            throw new IllegalArgumentException("Invalid classpath provider");
        return path->{
            InputStream stream=loader.getResourceAsStream(prefix+path);
            if(stream==null)throw new IOException("Packaged asset missing: "+path);
            return stream;
        };
    }
}
