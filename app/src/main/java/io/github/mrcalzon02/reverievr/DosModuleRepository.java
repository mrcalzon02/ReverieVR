package io.github.mrcalzon02.reverievr;

import android.content.Context;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Properties;

final class DosModuleRepository {
    private static final String ROOT = "dos-modules";
    private static final String METADATA = "module.properties";

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
