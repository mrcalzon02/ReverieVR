package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import android.widget.Toast;

import java.io.InputStream;
import java.security.MessageDigest;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

final class UpdateInstaller implements AutoCloseable {
    private final Activity activity;
    private final DownloadManager downloadManager;
    private final ExecutorService verifier = Executors.newSingleThreadExecutor();

    private long activeDownloadId = -1L;
    private UpdateChecker.Release activeRelease;
    private boolean receiverRegistered;

    private final BroadcastReceiver downloadReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction())) {
                return;
            }

            long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, -1L);
            if (id != activeDownloadId || activeRelease == null) {
                return;
            }

            Uri uri = downloadManager.getUriForDownloadedFile(id);
            if (uri == null) {
                Toast.makeText(
                    activity,
                    R.string.update_download_failed,
                    Toast.LENGTH_LONG
                ).show();
                clearActiveDownload();
                return;
            }

            verifyThenInstall(uri, activeRelease);
        }
    };

    UpdateInstaller(Activity activity) {
        this.activity = activity;
        this.downloadManager =
            (DownloadManager) activity.getSystemService(Context.DOWNLOAD_SERVICE);

        IntentFilter filter = new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            activity.registerReceiver(
                downloadReceiver,
                filter,
                Context.RECEIVER_NOT_EXPORTED
            );
        } else {
            activity.registerReceiver(downloadReceiver, filter);
        }
        receiverRegistered = true;
    }

    boolean canRequestPackageInstalls() {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.O
            || activity.getPackageManager().canRequestPackageInstalls();
    }

    void openInstallPermissionSettings() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            return;
        }

        Intent intent = new Intent(
            Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
            Uri.parse("package:" + activity.getPackageName())
        );
        activity.startActivity(intent);
    }

    void downloadAndInstall(UpdateChecker.Release release) {
        if (release == null || release.apkUrl == null) {
            return;
        }

        if (!canRequestPackageInstalls()) {
            Toast.makeText(
                activity,
                R.string.update_install_permission_needed,
                Toast.LENGTH_LONG
            ).show();
            openInstallPermissionSettings();
            return;
        }

        if (downloadManager == null) {
            Toast.makeText(
                activity,
                R.string.update_download_service_unavailable,
                Toast.LENGTH_LONG
            ).show();
            return;
        }

        String cleanVersion = release.version.replaceAll("[^A-Za-z0-9._-]", "_");
        String fileName = String.format(
            Locale.US,
            "ReverieVR-%s-%d.apk",
            cleanVersion,
            System.currentTimeMillis()
        );

        DownloadManager.Request request = new DownloadManager.Request(
            Uri.parse(release.apkUrl)
        );
        request.setTitle(activity.getString(R.string.update_download_title));
        request.setDescription(activity.getString(R.string.update_download_description));
        request.setNotificationVisibility(
            DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED
        );
        request.setDestinationInExternalFilesDir(
            activity,
            Environment.DIRECTORY_DOWNLOADS,
            fileName
        );

        activeRelease = release;
        activeDownloadId = downloadManager.enqueue(request);

        Toast.makeText(
            activity,
            R.string.update_download_started,
            Toast.LENGTH_SHORT
        ).show();
    }

    private void verifyThenInstall(Uri uri, UpdateChecker.Release release) {
        verifier.execute(() -> {
            boolean digestValid = verifyDigestIfPresent(uri, release.apkDigest);
            activity.runOnUiThread(() -> {
                if (!digestValid) {
                    Toast.makeText(
                        activity,
                        R.string.update_digest_failed,
                        Toast.LENGTH_LONG
                    ).show();
                    clearActiveDownload();
                    return;
                }

                launchPackageInstaller(uri);
                clearActiveDownload();
            });
        });
    }

    private boolean verifyDigestIfPresent(Uri uri, String digest) {
        if (digest == null || digest.trim().isEmpty()) {
            return true;
        }

        String normalized = digest.trim().toLowerCase(Locale.US);
        if (!normalized.startsWith("sha256:")) {
            return true;
        }

        String expected = normalized.substring("sha256:".length());
        try (
            InputStream input = activity.getContentResolver().openInputStream(uri)
        ) {
            if (input == null) {
                return false;
            }

            MessageDigest messageDigest = MessageDigest.getInstance("SHA-256");
            byte[] buffer = new byte[32 * 1024];
            int count;
            while ((count = input.read(buffer)) != -1) {
                messageDigest.update(buffer, 0, count);
            }

            return expected.equals(toHex(messageDigest.digest()));
        } catch (Exception exception) {
            return false;
        }
    }

    private void launchPackageInstaller(Uri uri) {
        Intent install = new Intent(Intent.ACTION_VIEW);
        install.setDataAndType(uri, "application/vnd.android.package-archive");
        install.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        install.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        try {
            activity.startActivity(install);
        } catch (Exception exception) {
            Toast.makeText(
                activity,
                R.string.update_installer_unavailable,
                Toast.LENGTH_LONG
            ).show();
        }
    }

    private static String toHex(byte[] bytes) {
        StringBuilder builder = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) {
            builder.append(String.format(Locale.US, "%02x", value & 0xff));
        }
        return builder.toString();
    }

    private void clearActiveDownload() {
        activeDownloadId = -1L;
        activeRelease = null;
    }

    @Override
    public void close() {
        verifier.shutdownNow();
        if (receiverRegistered) {
            try {
                activity.unregisterReceiver(downloadReceiver);
            } catch (IllegalArgumentException ignored) {
                // Receiver was already removed by the platform/activity teardown.
            }
            receiverRegistered = false;
        }
    }
}
