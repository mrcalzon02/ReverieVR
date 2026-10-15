package io.github.mrcalzon02.reverievr;

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;

final class BundledDosContentInstaller {
    static final String DOOM_MODULE_ID =
        "bundled-doom-shareware-1.9";

    static final String KEN_MODULE_ID =
        "bundled-kens-labyrinth-2.1";
    private static final String KEN_ASSET = "default-content/labfull.zip";
    private static final String KEN_SHA256 =
        "643aa0ca8d1a7d2c88afecd952da00f3b912be8b83c4c1d2f4ab9b50491fadfd";
    private static final String KEN_PAYLOAD_FILE = "labfull.zip";
    private static final String KEN_BOOTSTRAP_MARKER =
        ".reverie-ken-bootstrap-v1";

    private static final String DOOM_ASSET =
        "default-content/doom19s.zip";
    private static final String DOOM_SHA256 =
        "cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6";
    private static final String DOOM_WAD_SHA256 =
        "1d7d43be501e67d927e415e0b8f3e29c3bf33075e859721816f652a526cac771";
    private static final long DOOM_WAD_SIZE = 4196020L;
    private static final String CONTENT_DIRECTORY = "content";
    private static final String RUNTIME_DIRECTORY = "runtime";
    private static final String PAYLOAD_FILE = "doom19s.zip";
    private static final String BOOTSTRAP_MARKER =
        ".reverie-doom-bootstrap-v1";
    private static final String DOS_YML = "DOS.YML";
    private static final String BOOTSTRAP_BATCH =
        "DOSBOX.BAT";
    private static final String REVERIE_CONFIG =
        "REVERIE.CFG";

    private static final int BUFFER_SIZE = 64 * 1024;
    private static final int MAX_ARCHIVE_ENTRIES = 16;
    private static final long MAX_ARCHIVE_BYTES =
        8L * 1024L * 1024L;

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

        File directory = moduleDirectory();
        File payload = payloadFile(directory);
        DosGameModule existing =
            repository.findById(DOOM_MODULE_ID);

        if (!isVerifiedPayload(payload)) {
            if (existing != null || directory.exists()) {
                repository.delete(DOOM_MODULE_ID);
            }
            createContentDirectory(directory);
            payload = payloadFile(directory);
            copyVerifiedAsset(payload);
        }

        File runtime = ensureRuntime(directory, payload);
        writeLaunchMetadata(runtime);

        DosGameModule module =
            new DosGameModule(
                DOOM_MODULE_ID,
                "DOOM Shareware v1.9",
                PAYLOAD_FILE,
                runtime.getAbsolutePath(),
                existing == null || !BuiltInBindingProfiles.isDosProfileId(
                    existing.bindingProfileId)
                    ? BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE
                    : existing.bindingProfileId,
                0L
            );

        repository.save(module);
        ReverieLog.milestone(
            "BUNDLED_CONTENT",
            "Verified bundled DOOM Shareware runtime. installed="
                + isDoomInstalled(runtime)
        );
        return module;
    }


    // The original ZIP is verified before extraction and kept unchanged in
    // the APK. Runtime files are app-private, never a second Android package.
    DosGameModule ensureBundledKen() throws IOException {
        if (!BuildConfig.KENS_LABYRINTH_BUNDLED) {
            return null;
        }
        File directory = new File(repository.rootDirectory(), KEN_MODULE_ID);
        File payload = new File(
            new File(directory, CONTENT_DIRECTORY), KEN_PAYLOAD_FILE);
        DosGameModule existing = repository.findById(KEN_MODULE_ID);
        if (!payload.isFile()
            || !KEN_SHA256.equalsIgnoreCase(sha256(payload))) {
            if (directory.exists() && !deleteRecursively(directory)) {
                throw new IOException("Cannot remove invalid Ken's Labyrinth content.");
            }
            File content = payload.getParentFile();
            if (!content.mkdirs() && !content.isDirectory()) {
                throw new IOException("Cannot create Ken's Labyrinth content directory.");
            }
            copyVerifiedAsset(payload, KEN_ASSET, KEN_SHA256);
        }
        File runtime = new File(directory, RUNTIME_DIRECTORY);
        File marker = new File(runtime, KEN_BOOTSTRAP_MARKER);
        File executable = findKenExecutable(runtime);
        if (!marker.isFile() || executable == null || executable.length() == 0) {
            if (runtime.exists() && !deleteRecursively(runtime)) {
                throw new IOException("Cannot reset Ken's Labyrinth runtime.");
            }
            if (!runtime.mkdirs() && !runtime.isDirectory()) {
                throw new IOException("Cannot create Ken's Labyrinth runtime.");
            }
            try (InputStream input = new BufferedInputStream(
                    new FileInputStream(payload))) {
                BoundedZipExtractor.extract(input, runtime, 256,
                    64L * 1024L * 1024L);
            } catch (IOException failure) {
                deleteRecursively(runtime);
                throw failure;
            }
            executable = findKenExecutable(runtime);
            if (executable == null || executable.length() == 0) {
                deleteRecursively(runtime);
                throw new IOException(
                    "Original Ken's Labyrinth archive has no KEN.EXE.");
            }
            writeText(marker, "RVKEN1\nsha256=" + KEN_SHA256 + "\n");
        }
        String rootPath = runtime.getCanonicalPath() + File.separator;
        String executablePath = executable.getCanonicalPath();
        if (!executablePath.startsWith(rootPath)) {
            throw new IOException("Ken's Labyrinth launch path escapes runtime.");
        }
        String dosExecutable = executablePath.substring(rootPath.length())
            .replace(File.separatorChar, (char) 92);
        writeText(new File(runtime, DOS_YML),
            "run_path: C:" + (char) 92 + dosExecutable + "\n");
        DosGameModule module = new DosGameModule(
            KEN_MODULE_ID,
            "Ken's Labyrinth v2.1 (Freeware)",
            KEN_PAYLOAD_FILE,
            runtime.getAbsolutePath(),
            existing == null || !BuiltInBindingProfiles.isDosProfileId(
                existing.bindingProfileId)
                ? BuiltInBindingProfiles.ID_DOS_KENS_LABYRINTH
                : existing.bindingProfileId,
            0L
        );
        repository.save(module);
        ReverieLog.milestone("BUNDLED_CONTENT",
            "Verified and installed Ken's Labyrinth v2.1 freeware runtime.");
        return module;
    }

    private static File findKenExecutable(File runtime) {
        File rootExecutable = childFile(runtime, "KEN.EXE");
        if (rootExecutable != null) {
            return rootExecutable;
        }
        if (!runtime.isDirectory()) {
            return null;
        }
        File[] children = runtime.listFiles(File::isDirectory);
        if (children != null) {
            for (File child : children) {
                File candidate = childFile(child, "KEN.EXE");
                if (candidate != null) {
                    return candidate;
                }
            }
        }
        return null;
    }

    DosGameModule prepareForLaunch(
        DosGameModule module
    ) throws IOException {
        if (module != null && KEN_MODULE_ID.equals(module.id)) {
            return ensureBundledKen();
        }
        if (module == null
            || !DOOM_MODULE_ID.equals(module.id)) {
            return module;
        }

        File directory = moduleDirectory();
        File payload = payloadFile(directory);
        if (!isVerifiedPayload(payload)) {
            return ensureBundledDoom();
        }

        File runtime = ensureRuntime(directory, payload);
        writeLaunchMetadata(runtime);

        if (!runtime.getAbsolutePath().equals(
            module.contentPath
        )) {
            module =
                new DosGameModule(
                    module.id,
                    module.displayName,
                    module.originalFileName,
                    runtime.getAbsolutePath(),
                    module.bindingProfileId,
                    module.importedAtMillis
                );
            repository.save(module);
        }

        return module;
    }

    private File ensureRuntime(
        File directory,
        File payload
    ) throws IOException {
        File runtime =
            new File(directory, RUNTIME_DIRECTORY);
        File marker =
            new File(runtime, BOOTSTRAP_MARKER);

        if (!runtimeLooksValid(runtime)
            || !marker.isFile()) {
            deleteRecursively(runtime);
            if (!runtime.mkdirs()
                && !runtime.isDirectory()) {
                throw new IOException(
                    "Could not create bundled DOOM runtime directory."
                );
            }

            BoundedZipExtractor.ExtractionResult extraction;
            try (InputStream input =
                     new BufferedInputStream(
                         new FileInputStream(payload)
                     )) {
                extraction =
                    BoundedZipExtractor.extract(
                        input,
                        runtime,
                        MAX_ARCHIVE_ENTRIES,
                        MAX_ARCHIVE_BYTES
                    );
            }

            if (!runtimeLooksValid(runtime)) {
                deleteRecursively(runtime);
                throw new IOException(
                    "Bundled doom19s.zip did not contain the expected installer files."
                );
            }

            writeText(
                marker,
                "RVDOOM1\nsha256="
                    + DOOM_SHA256
                    + "\nentries="
                    + extraction.entryCount
                    + "\nbytes="
                    + extraction.totalBytes
                    + "\n"
            );

            ReverieLog.milestone(
                "BUNDLED_CONTENT",
                "Expanded verified DOOM Shareware installer into app-private runtime files="
                    + extraction.entryCount
                    + " bytes="
                    + extraction.totalBytes
            );
        }

        // Local app-private installation completes before DOSBox starts.
        // No interactive DEICE prompts, typed drive letters or keyboard.
        ensureInstalledGame(runtime);
        return runtime;
    }

    private void ensureInstalledGame(File runtime)
        throws IOException {
        if (isDoomInstalled(runtime)) {
            return;
        }
        File first = childFile(runtime, "DOOMS_19.1");
        File second = childFile(runtime, "DOOMS_19.2");
        File stage = new File(runtime, "DOOMS.reverie-partial");
        if (stage.exists() && !deleteRecursively(stage)) {
            throw new IOException("Could not clear incomplete DOOM staging files.");
        }
        try {
            BoundedZipExtractor.ExtractionResult extracted =
                DoomSharewareRuntimeExtractor.extract(first, second, stage);
            File contained = childDirectory(stage, "DOOMS");
            File source = contained == null ? stage : contained;
            File executable = childFile(source, "DOOM.EXE");
            File wad = childFile(source, "DOOM1.WAD");
            if (executable == null || executable.length() == 0
                || wad == null || wad.length() != DOOM_WAD_SIZE
                || !DOOM_WAD_SHA256.equalsIgnoreCase(sha256(wad))) {
                throw new IOException(
                    "Extracted DOOM shareware files failed integrity verification; "
                    + "the DOS installer will not be launched."
                );
            }

            File installed = new File(runtime, "DOOMS");
            if (installed.exists() && !deleteRecursively(installed)) {
                throw new IOException("Could not replace incomplete DOOM installation.");
            }
            if (!source.renameTo(installed)) {
                throw new IOException("Could not finalize installed DOOM game.");
            }
            ReverieLog.milestone(
                "BUNDLED_CONTENT",
                "Installed original shareware locally without DOS prompts; files="
                    + extracted.entryCount + " bytes=" + extracted.totalBytes
            );
        } finally {
            if (stage.exists() && !deleteRecursively(stage)) {
                ReverieLog.incident("BUNDLED_CONTENT",
                    "Could not clean DOOM staging directory.");
            }
        }
        if (!isDoomInstalled(runtime)) {
            throw new IOException("Installed DOOM integrity check did not pass.");
        }
    }

    private void writeLaunchMetadata(File runtime)
        throws IOException {
        File yml = new File(runtime, DOS_YML);
        File batch =
            new File(runtime, BOOTSTRAP_BATCH);
        File config =
            new File(runtime, REVERIE_CONFIG);

        writeText(
            config,
            DoomSharewareBootstrapPlan.doomConfig()
        );

        if (!isDoomInstalled(runtime)) {
            throw new IOException(
                "DOOM is not installed; refusing to enter a DOS installer."
            );
        }
        if (batch.exists() && !batch.delete()) {
            throw new IOException("Could not disable the obsolete DOS installer batch.");
        }
        File dooms = childDirectory(runtime, "DOOMS");
        writeText(new File(dooms, "DEFAULT.CFG"),
            DoomSharewareBootstrapPlan.doomConfig());
        writeText(yml, DoomSharewareBootstrapPlan.playYml());
    }

    private static boolean runtimeLooksValid(
        File runtime
    ) {
        return runtime.isDirectory()
            && childFile(runtime, "INSTALL.BAT") != null
            && childFile(runtime, "DEICE.EXE") != null
            && childFile(runtime, "DOOMS_19.DAT") != null
            && childFile(runtime, "DOOMS_19.1") != null
            && childFile(runtime, "DOOMS_19.2") != null;
    }

    private static boolean isDoomInstalled(
        File runtime
    ) throws IOException {
        File dooms = childDirectory(runtime, "DOOMS");
        // DEICE may create DOOM.EXE before unpacking all game data.
        // An executable alone must never promote an interrupted install
        // into the direct-play path.
        if (dooms == null) return false;
        File executable = childFile(dooms, "DOOM.EXE");
        File wad = childFile(dooms, "DOOM1.WAD");
        return executable != null && executable.length() > 0
            && wad != null && wad.length() == DOOM_WAD_SIZE
            && DOOM_WAD_SHA256.equalsIgnoreCase(sha256(wad));
    }

    private static File childFile(
        File parent,
        String name
    ) {
        return child(parent, name, false);
    }

    private static File childDirectory(
        File parent,
        String name
    ) {
        return child(parent, name, true);
    }

    private static File child(
        File parent,
        String name,
        boolean directory
    ) {
        if (parent == null || !parent.isDirectory()) {
            return null;
        }

        File[] children = parent.listFiles();
        if (children == null) {
            return null;
        }

        for (File child : children) {
            if (child.getName().equalsIgnoreCase(name)
                && (directory
                    ? child.isDirectory()
                    : child.isFile())) {
                return child;
            }
        }
        return null;
    }

    private File moduleDirectory() {
        return new File(
            repository.rootDirectory(),
            DOOM_MODULE_ID
        );
    }

    private static File payloadFile(File directory) {
        return new File(
            new File(directory, CONTENT_DIRECTORY),
            PAYLOAD_FILE
        );
    }

    private static void createContentDirectory(
        File directory
    ) throws IOException {
        File content =
            new File(directory, CONTENT_DIRECTORY);
        if (!content.mkdirs() && !content.isDirectory()) {
            throw new IOException(
                "Could not create bundled DOOM module directory."
            );
        }
    }

    private boolean isVerifiedPayload(File file)
        throws IOException {
        return file.isFile()
            && DOOM_SHA256.equalsIgnoreCase(
                sha256(file)
            );
    }

    private void copyVerifiedAsset(File destination)
        throws IOException {
        copyVerifiedAsset(destination, DOOM_ASSET, DOOM_SHA256);
    }

    private void copyVerifiedAsset(
        File destination, String assetName, String expectedSha256
    ) throws IOException {
        MessageDigest digest = sha256Digest();
        File partial =
            new File(
                destination.getParentFile(),
                destination.getName() + ".partial"
            );
        partial.delete();

        try (InputStream raw =
                 context.getAssets().open(assetName);
             BufferedInputStream input =
                 new BufferedInputStream(raw);
             FileOutputStream fileOutput =
                 new FileOutputStream(partial);
             BufferedOutputStream output =
                 new BufferedOutputStream(fileOutput)) {
            byte[] buffer = new byte[BUFFER_SIZE];
            int read;
            while ((read = input.read(buffer)) >= 0) {
                if (read <= 0) {
                    continue;
                }
                digest.update(buffer, 0, read);
                output.write(buffer, 0, read);
            }
            output.flush();
            fileOutput.getFD().sync();
        }

        String actual = hex(digest.digest());
        if (!expectedSha256.equals(actual)) {
            partial.delete();
            throw new IOException(
                "Bundled DOS archive checksum mismatch. Expected "
                    + expectedSha256
                    + " but found "
                    + actual
            );
        }

        if (destination.exists()
            && !destination.delete()) {
            partial.delete();
            throw new IOException(
                "Could not replace bundled DOS payload."
            );
        }
        if (!partial.renameTo(destination)) {
            partial.delete();
            throw new IOException(
                "Could not finalize bundled DOS payload."
            );
        }
    }

    private static void writeText(
        File file,
        String value
    ) throws IOException {
        byte[] bytes =
            value.getBytes(StandardCharsets.US_ASCII);
        try (FileOutputStream output =
                 new FileOutputStream(file)) {
            output.write(bytes);
            output.getFD().sync();
        }
    }

    private static String sha256(File file)
        throws IOException {
        MessageDigest digest = sha256Digest();

        try (BufferedInputStream input =
                 new BufferedInputStream(
                     new FileInputStream(file)
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

    private static boolean deleteRecursively(
        File file
    ) {
        if (file == null || !file.exists()) {
            return true;
        }

        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    if (!deleteRecursively(child)) {
                        return false;
                    }
                }
            }
        }

        return file.delete();
    }
}
