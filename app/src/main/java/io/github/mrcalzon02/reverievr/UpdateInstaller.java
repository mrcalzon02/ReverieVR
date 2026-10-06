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

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

final class UpdateInstaller implements AutoCloseable {
    private final Activity activity;
    private final DownloadManager downloadManager;
    private final Runnable failureFeedback;
    private final ExecutorService verifier =
        Executors.newSingleThreadExecutor();

    private long activeDownloadId = -1L;
    private UpdateChecker.Release activeRelease;
    private boolean receiverRegistered;

    private final BroadcastReceiver downloadReceiver =
        new BroadcastReceiver() {
            @Override
            public void onReceive(
                Context context,
                Intent intent
            ) {
                if (!DownloadManager
                        .ACTION_DOWNLOAD_COMPLETE
                        .equals(
                            intent.getAction()
                        )) {
                    return;
                }

                long id =
                    intent.getLongExtra(
                        DownloadManager
                            .EXTRA_DOWNLOAD_ID,
                        -1L
                    );

                if (id != activeDownloadId
                    || activeRelease == null) {
                    return;
                }

                Uri uri =
                    downloadManager
                        .getUriForDownloadedFile(
                            id
                        );

                if (uri == null) {
                    failVisible(
                        R.string
                            .update_download_failed
                    );
                    clearActiveDownload();
                    return;
                }

                verifyThenInstall(
                    uri,
                    activeRelease
                );
            }
        };

    UpdateInstaller(Activity activity) {
        this(activity, null);
    }

    UpdateInstaller(
        Activity activity,
        Runnable failureFeedback
    ) {
        this.activity = activity;
        this.failureFeedback =
            failureFeedback;
        this.downloadManager =
            (DownloadManager)
                activity.getSystemService(
                    Context.DOWNLOAD_SERVICE
                );

        IntentFilter filter =
            new IntentFilter(
                DownloadManager
                    .ACTION_DOWNLOAD_COMPLETE
            );

        if (Build.VERSION.SDK_INT
            >= Build.VERSION_CODES.TIRAMISU) {
            activity.registerReceiver(
                downloadReceiver,
                filter,
                Context.RECEIVER_NOT_EXPORTED
            );
        } else {
            activity.registerReceiver(
                downloadReceiver,
                filter
            );
        }

        receiverRegistered = true;
    }

    boolean canRequestPackageInstalls() {
        return Build.VERSION.SDK_INT
                < Build.VERSION_CODES.O
            || activity
                .getPackageManager()
                .canRequestPackageInstalls();
    }

    void openInstallPermissionSettings() {
        if (Build.VERSION.SDK_INT
            < Build.VERSION_CODES.O) {
            return;
        }

        Intent intent =
            new Intent(
                Settings
                    .ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                Uri.parse(
                    "package:"
                        + activity
                            .getPackageName()
                )
            );
        activity.startActivity(intent);
    }

    void downloadAndInstall(
        UpdateChecker.Release release
    ) {
        if (release == null
            || release.apkUrl == null
            || !UpdateChecker
                .isTrustedReleaseAssetUrl(
                    release.apkUrl
                )
            || !UpdateChecker
                .isValidSha256Digest(
                    release.apkDigest
                )) {
            failVisible(
                R.string
                    .update_package_metadata_invalid
            );
            return;
        }

        if (!BuildConfig
                .UPDATE_CHANNEL_ENABLED) {
            failVisible(
                R.string
                    .update_channel_not_signed
            );
            return;
        }

        if (!canRequestPackageInstalls()) {
            notifyFailure();
            Toast.makeText(
                activity,
                R.string
                    .update_install_permission_needed,
                Toast.LENGTH_LONG
            ).show();
            openInstallPermissionSettings();
            return;
        }

        if (downloadManager == null) {
            failVisible(
                R.string
                    .update_download_service_unavailable
            );
            return;
        }

        String cleanVersion =
            release.version
                .replaceAll(
                    "[^A-Za-z0-9._-]",
                    "_"
                );

        String fileName =
            String.format(
                Locale.US,
                "ReverieVR-%s-%d.apk",
                cleanVersion,
                System.currentTimeMillis()
            );

        DownloadManager.Request request =
            new DownloadManager.Request(
                Uri.parse(
                    release.apkUrl
                )
            );
        request.setTitle(
            activity.getString(
                R.string.update_download_title
            )
        );
        request.setDescription(
            activity.getString(
                R.string
                    .update_download_description
            )
        );
        request.setNotificationVisibility(
            DownloadManager.Request
                .VISIBILITY_VISIBLE_NOTIFY_COMPLETED
        );
        request.setDestinationInExternalFilesDir(
            activity,
            Environment.DIRECTORY_DOWNLOADS,
            fileName
        );

        activeRelease = release;

        try {
            activeDownloadId =
                downloadManager.enqueue(
                    request
                );
        } catch (RuntimeException exception) {
            clearActiveDownload();
            failVisible(
                R.string
                    .update_download_failed
            );
            return;
        }

        Toast.makeText(
            activity,
            R.string
                .update_download_started,
            Toast.LENGTH_SHORT
        ).show();
    }

    private void verifyThenInstall(
        Uri uri,
        UpdateChecker.Release release
    ) {
        verifier.execute(() -> {
            File verificationCopy = null;

            try {
                verificationCopy =
                    copyForVerification(uri);

                UpdatePackageVerifier.Result
                    result =
                        UpdatePackageVerifier
                            .verify(
                                activity,
                                verificationCopy,
                                release
                            );

                File finalVerificationCopy =
                    verificationCopy;

                activity.runOnUiThread(
                    () -> {
                        try {
                            if (!result.valid) {
                                notifyFailure();
                                Toast.makeText(
                                    activity,
                                    activity.getString(
                                        R.string
                                            .update_package_verification_failed_format,
                                        result.detail
                                    ),
                                    Toast.LENGTH_LONG
                                ).show();
                                clearActiveDownload();
                                return;
                            }

                            launchPackageInstaller(
                                uri
                            );
                            clearActiveDownload();
                        } finally {
                            deleteQuietly(
                                finalVerificationCopy
                            );
                        }
                    }
                );
            } catch (Exception exception) {
                deleteQuietly(
                    verificationCopy
                );

                activity.runOnUiThread(
                    () -> {
                        notifyFailure();
                        Toast.makeText(
                            activity,
                            activity.getString(
                                R.string
                                    .update_package_verification_failed_format,
                                exception
                                    .getClass()
                                    .getSimpleName()
                            ),
                            Toast.LENGTH_LONG
                        ).show();
                        clearActiveDownload();
                    }
                );
            }
        });
    }

    private File copyForVerification(
        Uri uri
    ) throws Exception {
        File file =
            File.createTempFile(
                "reverievr-update-",
                ".apk",
                activity.getCacheDir()
            );

        boolean complete = false;

        try (
            InputStream input =
                activity
                    .getContentResolver()
                    .openInputStream(uri);
            FileOutputStream output =
                new FileOutputStream(file)
        ) {
            if (input == null) {
                throw new IllegalStateException(
                    "Downloaded APK cannot be opened."
                );
            }

            byte[] buffer =
                new byte[32 * 1024];
            int count;

            while ((count =
                input.read(buffer)) != -1) {
                output.write(
                    buffer,
                    0,
                    count
                );
            }

            output.getFD().sync();
            complete = true;
            return file;
        } finally {
            if (!complete) {
                deleteQuietly(file);
            }
        }
    }

    private void launchPackageInstaller(
        Uri uri
    ) {
        Intent install =
            new Intent(
                Intent.ACTION_VIEW
            );
        install.setDataAndType(
            uri,
            "application/vnd.android.package-archive"
        );
        install.addFlags(
            Intent.FLAG_GRANT_READ_URI_PERMISSION
        );
        install.addFlags(
            Intent.FLAG_ACTIVITY_NEW_TASK
        );

        try {
            activity.startActivity(
                install
            );
        } catch (Exception exception) {
            failVisible(
                R.string
                    .update_installer_unavailable
            );
        }
    }

    private void failVisible(
        int messageResource
    ) {
        notifyFailure();
        Toast.makeText(
            activity,
            messageResource,
            Toast.LENGTH_LONG
        ).show();
    }

    private void notifyFailure() {
        if (failureFeedback == null) {
            return;
        }

        try {
            failureFeedback.run();
        } catch (RuntimeException ignored) {
            // Interaction feedback cannot block updater recovery.
        }
    }

    private static void deleteQuietly(
        File file
    ) {
        if (file == null
            || !file.exists()) {
            return;
        }

        try {
            if (!file.delete()) {
                file.deleteOnExit();
            }
        } catch (RuntimeException ignored) {
            // Verification cache cleanup must not block recovery.
        }
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
                activity
                    .unregisterReceiver(
                        downloadReceiver
                    );
            } catch (
                IllegalArgumentException ignored
            ) {
                // Receiver was already removed by Activity teardown.
            }

            receiverRegistered = false;
        }
    }
}
