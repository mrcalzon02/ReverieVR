package io.github.mrcalzon02.reverievr;

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;

final class BundledDosContentInstaller {
    private static final String DOOM_ASSET =
        "default-content/doom19s.zip";
    private static final String DOOM_MODULE_ID =
        "bundled-doom-shareware-1.9";
    private static final String DOOM_SHA256 =
        "cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6";
    private static final int BUFFER_SIZE = 64 * 1024;

    private final Context context;
    private final DosModuleRepository repository;

    BundledDosContentInstaller(
        Context context,
        DosModuleRepository repository
    ) {
        this.context = context.getApplicationContext();
        this.repository = repository;
    }

    DosGameModule ensureBundledDoom() throws IOException {
        if (!BuildConfig.DOOM_SHAREWARE_BUNDLED) {
            return null;
        }

        DosGameModule existing =
            repository.findById(DOOM_MODULE_ID);
        if (existing != null
            && existing.isContentPresent()
            && DOOM_SHA256.equalsIgnoreCase(
                sha256(existing.contentFile())
            )) {
            return existing;
        }

        if (existing != null) {
            repository.delete(DOOM_MODULE_ID);
        }

        File directory =
            new File(
                repository.rootDirectory(),
                DOOM_MODULE_ID
            );
        File contentDirectory =
            new File(directory, "content");
        if (!contentDirectory.mkdirs()
            && !contentDirectory.isDirectory()) {
            throw new IOException(
                "Could not create bundled DOOM module directory."
            );
        }

        File destination =
            new File(contentDirectory, "doom19s.zip");

        try {
            copyVerifiedAsset(destination);

            DosGameModule module =
                new DosGameModule(
                    DOOM_MODULE_ID,
                    "DOOM Shareware v1.9",
                    "doom19s.zip",
                    destination.getAbsolutePath(),
                    BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE,
                    0L
                );

            repository.save(module);
            ReverieLog.milestone(
                "BUNDLED_CONTENT",
                "Verified and registered bundled DOOM Shareware v1.9."
            );
            return module;
        } catch (IOException exception) {
            repository.delete(DOOM_MODULE_ID);
            ReverieLog.error(
                "BUNDLED_CONTENT",
                "Bundled DOOM Shareware verification/install failed.",
                exception
            );
            throw exception;
        }
    }

    private void copyVerifiedAsset(File destination)
        throws IOException {
        MessageDigest digest = sha256Digest();

        try (InputStream raw =
                 context.getAssets().open(DOOM_ASSET);
             BufferedInputStream input =
                 new BufferedInputStream(raw);
             BufferedOutputStream output =
                 new BufferedOutputStream(
                     new FileOutputStream(destination)
                 )) {
            byte[] buffer = new byte[BUFFER_SIZE];
            int read;
            while ((read = input.read(buffer)) >= 0) {
                if (read <= 0) {
                    continue;
                }
                digest.update(buffer, 0, read);
                output.write(buffer, 0, read);
            }
        }

        String actual = hex(digest.digest());
        if (!DOOM_SHA256.equals(actual)) {
            throw new IOException(
                "Bundled doom19s.zip checksum mismatch. Expected "
                    + DOOM_SHA256
                    + " but found "
                    + actual
            );
        }
    }

    private static String sha256(File file)
        throws IOException {
        MessageDigest digest = sha256Digest();

        try (BufferedInputStream input =
                 new BufferedInputStream(
                     new java.io.FileInputStream(file)
                 )) {
            byte[] buffer = new byte[BUFFER_SIZE];
            int read;
            while ((read = input.read(buffer)) >= 0) {
                if (read > 0) {
                    digest.update(buffer, 0, read);
                }
            }
        }

        return hex(digest.digest());
    }

    private static MessageDigest sha256Digest()
        throws IOException {
        try {
            return MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException exception) {
            throw new IOException(
                "SHA-256 is unavailable.",
                exception
            );
        }
    }

    private static String hex(byte[] bytes) {
        StringBuilder result =
            new StringBuilder(bytes.length * 2);
        for (byte value : bytes) {
            result.append(
                String.format(
                    Locale.US,
                    "%02x",
                    value & 0xff
                )
            );
        }
        return result.toString();
    }
}
