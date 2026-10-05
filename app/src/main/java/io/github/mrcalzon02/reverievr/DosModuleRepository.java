package io.github.mrcalzon02.reverievr;

import android.content.Context;

import java.io.File;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Properties;

final class DosModuleRepository {
    private static final String ROOT = "dos-modules";
    private static final String METADATA = "module.properties";
    private static final String CUSTOM_BINDING_PROFILE =
        "binding-profile.rvb";
    private static final int MAX_CUSTOM_BINDING_BYTES =
        64 * 1024;

    private final File root;

    DosModuleRepository(Context context) {
        root = new File(
            context.getFilesDir(),
            ROOT
        );
    }

    File rootDirectory() {
        return root;
    }

    synchronized void save(DosGameModule module)
        throws IOException {
        File directory = new File(root, module.id);
        if (!directory.isDirectory() && !directory.mkdirs()) {
            throw new IOException(
                "Could not create DOS module directory."
            );
        }

        Properties properties = new Properties();
        properties.setProperty("id", module.id);
        properties.setProperty(
            "displayName",
            module.displayName
        );
        properties.setProperty(
            "originalFileName",
            module.originalFileName
        );
        properties.setProperty(
            "contentPath",
            module.contentPath
        );
        properties.setProperty(
            "bindingProfileId",
            module.bindingProfileId
        );
        properties.setProperty(
            "importedAtMillis",
            Long.toString(module.importedAtMillis)
        );

        File metadata = new File(directory, METADATA);
        try (FileOutputStream output =
                 new FileOutputStream(metadata)) {
            properties.store(
                output,
                "ReverieVR DOS module"
            );
        }
    }

    synchronized List<DosGameModule> list() {
        if (!root.isDirectory()) {
            return Collections.emptyList();
        }

        File[] directories = root.listFiles(File::isDirectory);
        if (directories == null || directories.length == 0) {
            return Collections.emptyList();
        }

        List<DosGameModule> result = new ArrayList<>();
        for (File directory : directories) {
            DosGameModule module = read(directory);
            if (module != null && module.isContentPresent()) {
                result.add(module);
            }
        }

        result.sort(
            Comparator.comparingLong(
                (DosGameModule module) ->
                    module.importedAtMillis
            ).reversed()
        );
        return Collections.unmodifiableList(result);
    }

    synchronized DosGameModule findById(String id) {
        if (id == null || id.trim().isEmpty()) {
            return null;
        }
        return read(new File(root, id.trim()));
    }

    synchronized BindingProfile resolveBindingProfile(
        DosGameModule module
    ) {
        if (module == null) {
            return BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NONE
            );
        }

        File custom = new File(
            new File(root, module.id),
            CUSTOM_BINDING_PROFILE
        );
        if (custom.isFile()
            && custom.length() > 0L
            && custom.length() <= MAX_CUSTOM_BINDING_BYTES) {
            try {
                String encoded = readUtf8(custom);
                BindingProfile decoded =
                    BindingProfileCodec.decode(encoded);
                if (module.bindingProfileId.equals(decoded.id)
                    && BuiltInBindingProfiles.isDosProfileId(
                        decoded.id
                    )) {
                    return decoded;
                }
            } catch (IOException
                | RuntimeException ignored) {
                // Invalid custom data falls back to the selected built-in.
            }
        }

        return BuiltInBindingProfiles.byId(
            module.bindingProfileId
        );
    }

    synchronized boolean updateBindingProfileId(
        String moduleId,
        String profileId
    ) {
        if (moduleId == null
            || moduleId.trim().isEmpty()
            || !BuiltInBindingProfiles.isDosProfileId(profileId)) {
            return false;
        }

        DosGameModule current =
            read(new File(root, moduleId.trim()));
        if (current == null) {
            return false;
        }

        DosGameModule replacement =
            new DosGameModule(
                current.id,
                current.displayName,
                current.originalFileName,
                current.contentPath,
                profileId,
                current.importedAtMillis
            );

        try {
            save(replacement);
            clearCustomBindingProfile(current.id);
            return true;
        } catch (IOException exception) {
            return false;
        }
    }

    synchronized boolean saveCustomBindingProfile(
        String moduleId,
        BindingProfile profile
    ) {
        if (moduleId == null
            || moduleId.trim().isEmpty()
            || profile == null
            || !BuiltInBindingProfiles.isDosProfileId(profile.id)) {
            return false;
        }

        File directory =
            new File(root, moduleId.trim());
        DosGameModule module = read(directory);
        if (module == null
            || !module.bindingProfileId.equals(profile.id)) {
            return false;
        }

        File custom =
            new File(directory, CUSTOM_BINDING_PROFILE);
        byte[] bytes =
            BindingProfileCodec.encode(profile)
                .getBytes(StandardCharsets.UTF_8);
        if (bytes.length > MAX_CUSTOM_BINDING_BYTES) {
            return false;
        }

        try (FileOutputStream output =
                 new FileOutputStream(custom)) {
            output.write(bytes);
            output.getFD().sync();
            return true;
        } catch (IOException exception) {
            return false;
        }
    }

    synchronized boolean clearCustomBindingProfile(
        String moduleId
    ) {
        if (moduleId == null || moduleId.trim().isEmpty()) {
            return false;
        }

        File custom = new File(
            new File(root, moduleId.trim()),
            CUSTOM_BINDING_PROFILE
        );
        return !custom.exists() || custom.delete();
    }

    synchronized boolean hasCustomBindingProfile(
        String moduleId
    ) {
        if (moduleId == null || moduleId.trim().isEmpty()) {
            return false;
        }
        File custom = new File(
            new File(root, moduleId.trim()),
            CUSTOM_BINDING_PROFILE
        );
        return custom.isFile() && custom.length() > 0L;
    }

    synchronized boolean delete(String id) {
        if (id == null || id.trim().isEmpty()) {
            return false;
        }
        return deleteRecursively(
            new File(root, id.trim())
        );
    }

    synchronized void clearAll() {
        deleteRecursively(root);
    }

    private DosGameModule read(File directory) {
        File metadata = new File(directory, METADATA);
        if (!metadata.isFile()) {
            return null;
        }

        Properties properties = new Properties();
        try (FileInputStream input =
                 new FileInputStream(metadata)) {
            properties.load(input);

            return new DosGameModule(
                properties.getProperty("id"),
                properties.getProperty("displayName"),
                properties.getProperty("originalFileName"),
                properties.getProperty("contentPath"),
                properties.getProperty(
                    "bindingProfileId",
                    BuiltInBindingProfiles.ID_NONE
                ),
                Long.parseLong(
                    properties.getProperty(
                        "importedAtMillis",
                        "0"
                    )
                )
            );
        } catch (IOException
            | RuntimeException exception) {
            return null;
        }
    }

    private static String readUtf8(File file)
        throws IOException {
        try (FileInputStream input =
                 new FileInputStream(file);
             ByteArrayOutputStream output =
                 new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int total = 0;
            int read;

            while ((read = input.read(buffer)) != -1) {
                total += read;
                if (total > MAX_CUSTOM_BINDING_BYTES) {
                    throw new IOException(
                        "Custom binding profile is too large."
                    );
                }
                output.write(buffer, 0, read);
            }

            return new String(
                output.toByteArray(),
                StandardCharsets.UTF_8
            );
        }
    }

    private static boolean deleteRecursively(File file) {
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
