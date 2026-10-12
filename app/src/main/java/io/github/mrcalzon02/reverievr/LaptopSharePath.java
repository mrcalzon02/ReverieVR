package io.github.mrcalzon02.reverievr;

import java.io.IOException;

/** Validated SMB host/share-relative names. Never pass arbitrary UNC paths to SMBJ. */
final class LaptopSharePath {
    private LaptopSharePath() {}

    static String host(String value) throws IOException {
        if (value == null || !value.matches("[A-Za-z0-9][A-Za-z0-9._-]{0,252}")
            || value.endsWith(".") || value.contains(".."))
            throw new IOException("Enter a Windows host name or LAN IPv4 address.");
        return value;
    }

    static String share(String value) throws IOException {
        if (value == null || !value.matches("[A-Za-z0-9_$][A-Za-z0-9_ $.-]{0,79}")
            || value.endsWith(".") || value.endsWith(" "))
            throw new IOException("Enter a valid Windows shared-folder name.");
        return value;
    }

    static String segment(String value) throws IOException {
        if (value == null || value.length() == 0 || value.length() > 160
            || value.equals(".") || value.equals("..")
            || value.endsWith(".") || value.endsWith(" ")
            || value.matches(".*[\\\\/:*?\"<>|\\x00-\\x1f].*"))
            throw new IOException("Unsafe Windows share path component.");
        return value;
    }

    static String directory(String path) throws IOException {
        if (path == null || path.isEmpty()) return "";
        if (path.length() > 512 || path.startsWith("\\") || path.endsWith("\\"))
            throw new IOException("Unsafe shared-folder path.");
        String[] parts = path.split("\\\\", -1);
        for (String part : parts) segment(part);
        return path;
    }

    static String child(String parent, String name) throws IOException {
        String base = directory(parent);
        String part = segment(name);
        return directory(base.isEmpty() ? part : base + "\\" + part);
    }

    static String parent(String path) throws IOException {
        String safe = directory(path);
        int last = safe.lastIndexOf('\\');
        return last < 0 ? "" : safe.substring(0, last);
    }

    static String fileName(String path) throws IOException {
        String safe=directory(path);
        if(safe.isEmpty())throw new IOException("Select a file.");
        return safe.substring(safe.lastIndexOf('\\')+1);
    }
}
