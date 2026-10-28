package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Bounded, deterministic desktop authoring exchange. This is not yet a runtime asset. */
final class SceneDocument {
    static final int MAX_OBJECTS = 512;
    static final class Piece {
        String id, kind, asset;
        double x, y, z, yaw, scale;
        Piece(String id, String kind, String asset, double x, double y, double z, double yaw, double scale) {
            this.id=id; this.kind=kind; this.asset=asset;
            this.x=x; this.y=y; this.z=z; this.yaw=yaw; this.scale=scale;
            validate();
        }
        Piece copy() { return new Piece(id,kind,asset,x,y,z,yaw,scale); }
        void validate() {
            if (!id.matches("[A-Za-z0-9_-]{1,48}") ||
                !(kind.equals("box") || kind.equals("cylinder") || kind.equals("obj")) ||
                asset.contains("\t") || asset.contains("\n") || asset.contains("\r") ||
                (kind.equals("obj") && !safeAsset(asset)) ||
                (!kind.equals("obj") && !asset.equals("-")))
                throw new IllegalArgumentException("Invalid scene object identity/asset: " + id);
            for (double v : new double[]{x,y,z,yaw,scale})
                if (!Double.isFinite(v) || Math.abs(v)>100000) throw new IllegalArgumentException("Nonfinite or oversized transform");
            if (scale <= 0 || scale > 500) throw new IllegalArgumentException("Scale must be positive and <= 500");
        }
    }
    String environment = "white_room";
    final List<Piece> pieces = new ArrayList<>();

    static boolean safeAsset(String name) {
        if (name.isBlank() || name.startsWith("/") || name.startsWith("\\") || name.contains("\\") ||
            name.contains(":") || !name.toLowerCase(Locale.ROOT).endsWith(".obj")) return false;
        Path path;
        try { path=Path.of(name); } catch (RuntimeException e) { return false; }
        if (path.isAbsolute()) return false;
        for (Path segment : path) if (segment.toString().equals("..") || segment.toString().equals(".")) return false;
        return name.length()<=256;
    }
    String encode() {
        if (pieces.size()>MAX_OBJECTS) throw new IllegalArgumentException("Too many objects");
        if (!validEnvironment(environment)) throw new IllegalArgumentException("Invalid environment");
        StringBuilder result = new StringBuilder("REVSCENE\t1\nenvironment\t").append(environment).append('\n');
        java.util.HashSet<String> ids=new java.util.HashSet<>();
        for (Piece p : pieces) {
            p.validate();
            if (!ids.add(p.id)) throw new IllegalArgumentException("Duplicate object ID: " + p.id);
            result.append("piece\t").append(p.id).append('\t').append(p.kind).append('\t')
                .append(p.asset).append('\t').append(p.x).append('\t').append(p.y)
                .append('\t').append(p.z).append('\t').append(p.yaw).append('\t')
                .append(p.scale).append('\n');
        }
        return result.toString();
    }
    static boolean validEnvironment(String id) {
        for (HomeEnvironment environment : HomeEnvironment.values())
            if (environment.preferenceValue.equals(id)) return true;
        return false;
    }
    static SceneDocument decode(String content) throws IOException {
        if (content.length()>150000) throw new IOException("Scene exceeds size limit");
        String[] lines=content.split("\n",-1);
        if (lines.length<3 || !lines[0].equals("REVSCENE\t1")) throw new IOException("Scene magic/version invalid");
        String[] env=lines[1].split("\t",-1);
        if (env.length!=2 || !env[0].equals("environment") || !validEnvironment(env[1]))
            throw new IOException("Unsupported scene environment");
        SceneDocument result=new SceneDocument();result.environment=env[1];
        java.util.HashSet<String> ids=new java.util.HashSet<>();
        for (int i=2;i<lines.length;i++) {
            if (lines[i].isEmpty() && i==lines.length-1) continue;
            String[] f=lines[i].split("\t",-1);
            if (f.length!=9 || !f[0].equals("piece") || result.pieces.size()>=MAX_OBJECTS)
                throw new IOException("Invalid scene record at line " +(i+1));
            try {
                Piece p=new Piece(f[1],f[2],f[3],Double.parseDouble(f[4]),
                    Double.parseDouble(f[5]),Double.parseDouble(f[6]),
                    Double.parseDouble(f[7]),Double.parseDouble(f[8]));
                if (!ids.add(p.id)) throw new IOException("Duplicate object ID: "+p.id);
                result.pieces.add(p);
            } catch (IllegalArgumentException error) { throw new IOException("Invalid object at line "+(i+1),error); }
        }
        return result;
    }
    static SceneDocument load(Path file) throws IOException {
        if (!Files.exists(file)) return new SceneDocument();
        if (Files.size(file)>150000) throw new IOException("Scene exceeds size limit");
        return decode(Files.readString(file, StandardCharsets.UTF_8));
    }
    void save(Path file) throws IOException {
        String data=encode();
        Files.createDirectories(file.getParent());
        Path temp=Files.createTempFile(file.getParent(),".rvscene-",".tmp");
        try {
            Files.writeString(temp,data,StandardCharsets.UTF_8);
            try { Files.move(temp,file,StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING); }
            catch (AtomicMoveNotSupportedException e) { Files.move(temp,file,StandardCopyOption.REPLACE_EXISTING); }
        } finally { Files.deleteIfExists(temp); }
    }
}
