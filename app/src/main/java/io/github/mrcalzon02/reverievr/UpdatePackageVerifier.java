package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Build;

import java.io.File;
import java.io.FileInputStream;
import java.security.MessageDigest;
import java.util.Locale;

final class UpdatePackageVerifier {
    static final class Result {
        final boolean valid;
        final String detail;

        private Result(
            boolean valid,
            String detail
        ) {
            this.valid = valid;
            this.detail = detail;
        }

        static Result ok() {
            return new Result(
                true,
                "Update package verified."
            );
        }

        static Result fail(String detail) {
            return new Result(
                false,
                detail == null
                    || detail.trim().isEmpty()
                    ? "Update package verification failed."
                    : detail.trim()
            );
        }
    }

    private UpdatePackageVerifier() {}

    static Result verify(
        Activity activity,
        File apkFile,
        UpdateChecker.Release release
    ) {
        if (activity == null
            || apkFile == null
            || release == null) {
            return Result.fail(
                "Missing update verification input."
            );
        }

        if (!apkFile.isFile()
            || apkFile.length() <= 0L) {
            return Result.fail(
                "Downloaded APK is missing or empty."
            );
        }

        if (!UpdateChecker.isTrustedReleaseAssetUrl(
                release.apkUrl
            )) {
            return Result.fail(
                "Update source is not the trusted ReverieVR GitHub release path."
            );
        }

        String expectedDigest =
            normalizeSha256Digest(
                release.apkDigest
            );
        if (expectedDigest == null) {
            return Result.fail(
                "GitHub release is missing a valid SHA-256 asset digest."
            );
        }

        String actualDigest =
            sha256File(apkFile);
        if (actualDigest == null
            || !expectedDigest.equals(
                actualDigest
            )) {
            return Result.fail(
                "Downloaded APK SHA-256 does not match GitHub release metadata."
            );
        }

        PackageManager packageManager =
            activity.getPackageManager();
        if (packageManager == null) {
            return Result.fail(
                "Android PackageManager is unavailable."
            );
        }

        PackageInfo candidate =
            getArchivePackageInfo(
                packageManager,
                apkFile
            );
        if (candidate == null) {
            return Result.fail(
                "Android could not parse the downloaded APK."
            );
        }

        String expectedPackage =
            activity.getPackageName();
        if (expectedPackage == null
            || !expectedPackage.equals(
                candidate.packageName
            )) {
            return Result.fail(
                "Downloaded APK package ID does not match ReverieVR."
            );
        }

        PackageInfo installed;
        try {
            installed =
                getInstalledPackageInfo(
                    packageManager,
                    expectedPackage
                );
        } catch (
            PackageManager.NameNotFoundException exception
        ) {
            return Result.fail(
                "Installed ReverieVR package metadata is unavailable."
            );
        }

        long installedVersionCode =
            versionCode(installed);
        long candidateVersionCode =
            versionCode(candidate);

        if (candidateVersionCode
            <= installedVersionCode) {
            return Result.fail(
                "Downloaded APK is not newer than the installed ReverieVR build."
            );
        }

        int expectedVersionCode =
            UpdateChecker.versionCodeForPhoneTest(
                release.phoneTestRun,
                release.phoneTestAttempt
            );
        if (expectedVersionCode <= 0
            || candidateVersionCode
                != expectedVersionCode) {
            return Result.fail(
                "Downloaded APK versionCode does not match its GitHub phone-test tag."
            );
        }

        String pinnedSigner =
            normalizeSha256Fingerprint(
                BuildConfig
                    .DISTRIBUTION_SIGNER_SHA256
            );
        if (pinnedSigner == null) {
            return Result.fail(
                "This ReverieVR build has no pinned distribution signing identity."
            );
        }

        String installedSigner =
            singleCurrentSignerSha256(
                installed
            );
        if (!pinnedSigner.equals(
                installedSigner
            )) {
            return Result.fail(
                "Installed ReverieVR signing certificate does not match the pinned distribution identity."
            );
        }

        String candidateSigner =
            singleCurrentSignerSha256(
                candidate
            );
        if (!candidateSignerAcceptable(
                pinnedSigner,
                candidateSigner
            )) {
            return Result.fail(
                "Downloaded APK signing certificate does not match the installed ReverieVR identity."
            );
        }

        if (candidateSigner == null) {
            ReverieLog.dev(
                "UPDATE",
                "Android did not expose the downloaded APK signer through "
                    + "PackageManager archive parsing; digest/package/version "
                    + "checks passed, so final signer continuity is delegated "
                    + "to Android Package Installer."
            );
        }

        return Result.ok();
    }

    static boolean candidateSignerAcceptable(
        String pinnedSigner,
        String candidateSigner
    ) {
        if (pinnedSigner == null
            || pinnedSigner.trim().isEmpty()) {
            return false;
        }

        return candidateSigner == null
            || pinnedSigner.equals(candidateSigner);
    }

    static String normalizeSha256Digest(
        String value
    ) {
        if (!UpdateChecker
                .isValidSha256Digest(value)) {
            return null;
        }

        return value
            .trim()
            .substring(
                "sha256:".length()
            )
            .toLowerCase(Locale.US);
    }

    static String normalizeSha256Fingerprint(
        String value
    ) {
        if (value == null) {
            return null;
        }

        String normalized =
            value
                .replace(":", "")
                .replace(" ", "")
                .trim()
                .toLowerCase(Locale.US);

        if (!normalized.matches(
                "^[0-9a-f]{64}$"
            )) {
            return null;
        }
        return normalized;
    }

    private static PackageInfo
        getArchivePackageInfo(
            PackageManager packageManager,
            File apkFile
        ) {
        int flags =
            Build.VERSION.SDK_INT
                >= Build.VERSION_CODES.P
                ? (
                    PackageManager.GET_SIGNING_CERTIFICATES
                        | PackageManager.GET_SIGNATURES
                )
                : PackageManager.GET_SIGNATURES;

        PackageInfo packageInfo =
            packageManager
                .getPackageArchiveInfo(
                    apkFile.getAbsolutePath(),
                    flags
                );

        if (packageInfo != null
            && packageInfo.applicationInfo != null) {
            packageInfo
                .applicationInfo
                .sourceDir =
                    apkFile.getAbsolutePath();
            packageInfo
                .applicationInfo
                .publicSourceDir =
                    apkFile.getAbsolutePath();
        }

        return packageInfo;
    }

    private static PackageInfo
        getInstalledPackageInfo(
            PackageManager packageManager,
            String packageName
        )
        throws PackageManager
            .NameNotFoundException {
        int flags =
            Build.VERSION.SDK_INT
                >= Build.VERSION_CODES.P
                ? (
                    PackageManager.GET_SIGNING_CERTIFICATES
                        | PackageManager.GET_SIGNATURES
                )
                : PackageManager.GET_SIGNATURES;

        return packageManager.getPackageInfo(
            packageName,
            flags
        );
    }

    private static long versionCode(
        PackageInfo packageInfo
    ) {
        if (Build.VERSION.SDK_INT
            >= Build.VERSION_CODES.P) {
            return packageInfo
                .getLongVersionCode();
        }

        return packageInfo.versionCode;
    }

    private static String
        singleCurrentSignerSha256(
            PackageInfo packageInfo
        ) {
        Signature[] signatures = null;

        if (Build.VERSION.SDK_INT
            >= Build.VERSION_CODES.P
            && packageInfo.signingInfo != null) {
            signatures =
                packageInfo
                    .signingInfo
                    .getApkContentsSigners();
        }

        if (signatures == null
            || signatures.length == 0) {
            signatures =
                packageInfo.signatures;
        }

        if (signatures == null
            || signatures.length != 1
            || signatures[0] == null) {
            return null;
        }

        return sha256Bytes(
            signatures[0].toByteArray()
        );
    }

    private static String sha256File(
        File file
    ) {
        try (
            FileInputStream input =
                new FileInputStream(file)
        ) {
            MessageDigest digest =
                MessageDigest.getInstance(
                    "SHA-256"
                );
            byte[] buffer =
                new byte[32 * 1024];

            int count;
            while ((count =
                input.read(buffer)) != -1) {
                digest.update(
                    buffer,
                    0,
                    count
                );
            }

            return toHex(
                digest.digest()
            );
        } catch (Exception exception) {
            return null;
        }
    }

    private static String sha256Bytes(
        byte[] bytes
    ) {
        if (bytes == null) {
            return null;
        }

        try {
            MessageDigest digest =
                MessageDigest.getInstance(
                    "SHA-256"
                );
            return toHex(
                digest.digest(bytes)
            );
        } catch (Exception exception) {
            return null;
        }
    }

    private static String toHex(
        byte[] bytes
    ) {
        StringBuilder builder =
            new StringBuilder(
                bytes.length * 2
            );
        for (byte value : bytes) {
            builder.append(
                String.format(
                    Locale.US,
                    "%02x",
                    value & 0xff
                )
            );
        }
        return builder.toString();
    }
}
