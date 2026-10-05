package io.github.mrcalzon02.reverievr;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.LocationManager;
import android.net.Uri;
import android.os.Build;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Arrays;
import java.util.Comparator;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

final class DiagnosticBundleExporter {
    private static final int BUFFER_SIZE = 64 * 1024;
    private static final String DIAGNOSTIC_ID_PATTERN =
        "^revdiag-[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-"
            + "[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-"
            + "[0-9a-fA-F]{12}$";
    private static final long PRIVATE_BUNDLE_RETENTION_MILLIS =
        30L * 24L * 60L * 60L * 1000L;

    private final Context context;

    DiagnosticBundleExporter(Context context) {
        this.context = context.getApplicationContext();
    }

    void export(Uri destination) throws IOException {
        if (destination == null) {
            throw new IOException(
                "Diagnostic export destination is missing."
            );
        }

        OutputStream raw =
            context.getContentResolver()
                .openOutputStream(destination, "w");
        if (raw == null) {
            throw new IOException(
                "Android could not open the diagnostic export destination."
            );
        }

        writeBundle(
            raw,
            "",
            true
        );
    }

    File createSubmissionBundle(
        String diagnosticId,
        boolean includeLogs
    ) throws IOException {
        String safeId =
            diagnosticId == null
                ? ""
                : diagnosticId.trim();
        if (!safeId.matches(
                DIAGNOSTIC_ID_PATTERN
            )) {
            throw new IOException(
                "Diagnostic ID is invalid."
            );
        }

        File directory =
            new File(
                context.getFilesDir(),
                "diagnostic-outbox"
            );
        if (!directory.isDirectory()
            && !directory.mkdirs()) {
            throw new IOException(
                "Could not create diagnostic outbox."
            );
        }

        pruneOldSubmissionBundles(directory);

        File bundle =
            new File(
                directory,
                safeId + ".zip"
            );

        try (FileOutputStream output =
                 new FileOutputStream(bundle, false)) {
            writeBundle(
                output,
                safeId,
                includeLogs
            );
        } catch (IOException exception) {
            if (bundle.isFile()
                && !bundle.delete()) {
                ReverieLog.incident(
                    "DIAGNOSTICS",
                    "Could not remove incomplete submission bundle "
                        + bundle.getAbsolutePath()
                );
            }
            throw exception;
        }

        return bundle;
    }

    private void writeBundle(
        OutputStream raw,
        String diagnosticId,
        boolean includeLogs
    ) throws IOException {
        try (ZipOutputStream zip =
                 new ZipOutputStream(
                     new BufferedOutputStream(raw)
                 )) {
            addManifest(
                zip,
                diagnosticId,
                includeLogs
            );
            if (includeLogs) {
                addLogFiles(zip);
            }
        }
    }

    private void addManifest(
        ZipOutputStream zip,
        String diagnosticId,
        boolean includeLogs
    ) throws IOException {
        StringBuilder manifest =
            new StringBuilder(1600);

        manifest.append("ReverieVR diagnostic bundle\n")
            .append("created=")
            .append(Instant.now())
            .append('\n');

        if (diagnosticId != null
            && !diagnosticId.trim().isEmpty()) {
            manifest.append("diagnosticId=")
                .append(diagnosticId.trim())
                .append('\n');
        }

        manifest.append("appVersion=")
            .append(BuildConfig.VERSION_NAME)
            .append('\n')
            .append("versionCode=")
            .append(BuildConfig.VERSION_CODE)
            .append('\n')
            .append("buildType=")
            .append(BuildConfig.BUILD_TYPE)
            .append('\n')
            .append("dosRuntimeBuilt=")
            .append(BuildConfig.DOS_RUNTIME_BUILT)
            .append('\n')
            .append("loggingMode=")
            .append(ReverieLog.getMode().name())
            .append('\n')
            .append("logsIncluded=")
            .append(includeLogs)
            .append('\n')
            .append("manufacturer=")
            .append(Build.MANUFACTURER)
            .append('\n')
            .append("model=")
            .append(Build.MODEL)
            .append('\n')
            .append("device=")
            .append(Build.DEVICE)
            .append('\n')
            .append("androidRelease=")
            .append(Build.VERSION.RELEASE)
            .append('\n')
            .append("apiLevel=")
            .append(Build.VERSION.SDK_INT)
            .append('\n')
            .append("abi=")
            .append(
                Build.SUPPORTED_ABIS.length == 0
                    ? "unknown"
                    : Build.SUPPORTED_ABIS[0]
            )
            .append('\n')
            .append("bluetoothAdapter=")
            .append(bluetoothAdapterState())
            .append('\n')
            .append("locationServices=")
            .append(locationServicesState())
            .append('\n')
            .append("bleLegacyLocationGate=")
            .append(legacyBleLocationGateState())
            .append('\n')
            .append("permissionBluetoothScan=")
            .append(
                permissionState(
                    Manifest.permission.BLUETOOTH_SCAN,
                    Build.VERSION_CODES.S
                )
            )
            .append('\n')
            .append("permissionBluetoothConnect=")
            .append(
                permissionState(
                    Manifest.permission.BLUETOOTH_CONNECT,
                    Build.VERSION_CODES.S
                )
            )
            .append('\n')
            .append("permissionFineLocation=")
            .append(
                permissionState(
                    Manifest.permission.ACCESS_FINE_LOCATION,
                    Build.VERSION_CODES.M
                )
            )
            .append('\n')
            .append("permissionCoarseLocation=")
            .append(
                permissionState(
                    Manifest.permission.ACCESS_COARSE_LOCATION,
                    Build.VERSION_CODES.M
                )
            )
            .append('\n')
            .append('\n')
            .append("Privacy note: development logs may contain device names, ")
            .append("local filenames, module names, controller state, timing, ")
            .append("and diagnostic application events. Review before sharing.\n");

        ZipEntry entry =
            new ZipEntry("manifest.txt");
        zip.putNextEntry(entry);
        zip.write(
            manifest.toString()
                .getBytes(StandardCharsets.UTF_8)
        );
        zip.closeEntry();
    }

    private String bluetoothAdapterState() {
        BluetoothManager manager =
            (BluetoothManager) context.getSystemService(
                Context.BLUETOOTH_SERVICE
            );
        if (manager == null) {
            return "unavailable";
        }

        BluetoothAdapter adapter =
            manager.getAdapter();
        if (adapter == null) {
            return "unavailable";
        }

        try {
            return adapter.isEnabled()
                ? "enabled"
                : "disabled";
        } catch (SecurityException exception) {
            return "unknown-permission";
        }
    }

    private String locationServicesState() {
        LocationManager manager =
            (LocationManager) context.getSystemService(
                Context.LOCATION_SERVICE
            );
        if (manager == null) {
            return "unavailable";
        }

        try {
            if (Build.VERSION.SDK_INT
                >= Build.VERSION_CODES.P) {
                return manager.isLocationEnabled()
                    ? "enabled"
                    : "disabled";
            }

            boolean enabled =
                manager.isProviderEnabled(
                    LocationManager.GPS_PROVIDER
                )
                    || manager.isProviderEnabled(
                        LocationManager.NETWORK_PROVIDER
                    );
            return enabled
                ? "enabled"
                : "disabled";
        } catch (RuntimeException exception) {
            return "unknown";
        }
    }

    private String legacyBleLocationGateState() {
        if (Build.VERSION.SDK_INT
            >= Build.VERSION_CODES.S) {
            return "not-applicable";
        }

        String permission =
            permissionState(
                Manifest.permission.ACCESS_FINE_LOCATION,
                Build.VERSION_CODES.M
            );
        String location =
            locationServicesState();

        if ("granted".equals(permission)
            && "enabled".equals(location)) {
            return "open";
        }
        if ("denied".equals(permission)
            || "disabled".equals(location)) {
            return "closed";
        }
        return "unknown";
    }

    private String permissionState(
        String permission,
        int requiredFromApi
    ) {
        if (Build.VERSION.SDK_INT
            < requiredFromApi) {
            return "not-required";
        }

        return context.checkSelfPermission(
            permission
        ) == PackageManager.PERMISSION_GRANTED
            ? "granted"
            : "denied";
    }

    private void addLogFiles(ZipOutputStream zip)
        throws IOException {
        String path =
            ReverieLog.getLogDirectoryPath();
        if (path == null || path.trim().isEmpty()) {
            return;
        }

        File directory = new File(path);
        File[] files = directory.listFiles(
            file ->
                file.isFile()
                    && file.getName()
                        .startsWith("reverie-")
        );
        if (files == null || files.length == 0) {
            return;
        }

        Arrays.sort(
            files,
            Comparator.comparing(File::getName)
        );

        byte[] buffer = new byte[BUFFER_SIZE];
        for (File file : files) {
            ZipEntry entry =
                new ZipEntry(
                    "logs/" + file.getName()
                );
            zip.putNextEntry(entry);

            try (BufferedInputStream input =
                     new BufferedInputStream(
                         new FileInputStream(file)
                     )) {
                int read;
                while ((read = input.read(buffer)) >= 0) {
                    if (read > 0) {
                        zip.write(buffer, 0, read);
                    }
                }
            }

            zip.closeEntry();
        }
    }

    private void pruneOldSubmissionBundles(
        File directory
    ) {
        File[] files =
            directory.listFiles(
                file ->
                    file.isFile()
                        && file.getName().startsWith("revdiag-")
                        && file.getName().endsWith(".zip")
            );
        if (files == null) {
            return;
        }

        long cutoff =
            System.currentTimeMillis()
                - PRIVATE_BUNDLE_RETENTION_MILLIS;
        for (File file : files) {
            if (file.lastModified() > 0
                && file.lastModified() < cutoff
                && !file.delete()) {
                ReverieLog.dev(
                    "DIAGNOSTICS",
                    "Could not prune expired private diagnostic bundle "
                        + file.getAbsolutePath()
                );
            }
        }
    }
}
