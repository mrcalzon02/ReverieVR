package io.github.mrcalzon02.reverievr;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

final class UpdateChecker implements AutoCloseable {
    static final String RELEASES_API =
        "https://api.github.com/repos/mrcalzon02/ReverieVR/releases?per_page=50";

    private static final Pattern PHONE_TEST_TAG =
        Pattern.compile("^phone-test-(\\d+)-(\\d+)$");

    interface Callback {
        void onResult(Result result);
    }

    static final class Release {
        final String version;
        final String title;
        final String notes;
        final String releasePageUrl;
        final String apkUrl;
        final String apkDigest;
        final int phoneTestRun;
        final int phoneTestAttempt;

        Release(
            String version,
            String title,
            String notes,
            String releasePageUrl,
            String apkUrl,
            String apkDigest,
            int phoneTestRun,
            int phoneTestAttempt
        ) {
            this.version = version;
            this.title = title;
            this.notes = notes;
            this.releasePageUrl = releasePageUrl;
            this.apkUrl = apkUrl;
            this.apkDigest = apkDigest;
            this.phoneTestRun = phoneTestRun;
            this.phoneTestAttempt = phoneTestAttempt;
        }
    }

    static final class Result {
        enum State {
            UPDATE_AVAILABLE,
            UP_TO_DATE,
            NO_RELEASES,
            RELEASE_WITHOUT_APK,
            ERROR
        }

        final State state;
        final Release release;
        final String message;

        Result(State state, Release release, String message) {
            this.state = state;
            this.release = release;
            this.message = message;
        }
    }

    private static final class PhoneTestTag {
        final int run;
        final int attempt;

        PhoneTestTag(int run, int attempt) {
            this.run = run;
            this.attempt = attempt;
        }

        static PhoneTestTag parse(String value) {
            if (value == null) {
                return null;
            }

            Matcher matcher =
                PHONE_TEST_TAG.matcher(value.trim());
            if (!matcher.matches()) {
                return null;
            }

            try {
                return new PhoneTestTag(
                    Integer.parseInt(matcher.group(1)),
                    Integer.parseInt(matcher.group(2))
                );
            } catch (NumberFormatException exception) {
                return null;
            }
        }
    }

    private final ExecutorService executor =
        Executors.newSingleThreadExecutor();

    void check(
        String currentVersion,
        int currentPhoneTestRun,
        int currentPhoneTestAttempt,
        Callback callback
    ) {
        executor.execute(
            () -> callback.onResult(
                checkBlocking(
                    currentVersion,
                    currentPhoneTestRun,
                    currentPhoneTestAttempt
                )
            )
        );
    }

    private Result checkBlocking(
        String currentVersion,
        int currentPhoneTestRun,
        int currentPhoneTestAttempt
    ) {
        HttpURLConnection connection = null;

        try {
            URL endpoint = new URL(RELEASES_API);
            connection =
                (HttpURLConnection) endpoint.openConnection();
            connection.setConnectTimeout(8000);
            connection.setReadTimeout(8000);
            connection.setRequestProperty(
                "Accept",
                "application/vnd.github+json"
            );
            connection.setRequestProperty(
                "X-GitHub-Api-Version",
                "2022-11-28"
            );
            connection.setRequestProperty(
                "User-Agent",
                "ReverieVR/" + currentVersion
            );

            int status = connection.getResponseCode();
            if (status == HttpURLConnection.HTTP_NOT_FOUND) {
                return new Result(
                    Result.State.NO_RELEASES,
                    null,
                    "No published ReverieVR release exists yet."
                );
            }

            if (status < 200 || status >= 300) {
                return new Result(
                    Result.State.ERROR,
                    null,
                    "GitHub returned HTTP " + status + "."
                );
            }

            return evaluateReleases(
                readAll(connection.getInputStream()),
                currentVersion,
                currentPhoneTestRun,
                currentPhoneTestAttempt
            );
        } catch (Exception exception) {
            return new Result(
                Result.State.ERROR,
                null,
                exception.getClass().getSimpleName()
                    + ": "
                    + safeMessage(exception)
            );
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
    }

    static Result evaluateReleases(
        String json,
        String currentVersion,
        int currentPhoneTestRun,
        int currentPhoneTestAttempt
    ) throws Exception {
        JSONArray releases = new JSONArray(json);
        if (releases.length() == 0) {
            return new Result(
                Result.State.NO_RELEASES,
                null,
                "No published ReverieVR release exists yet."
            );
        }

        Release newerWithoutApk = null;

        for (int index = 0; index < releases.length(); index++) {
            JSONObject releaseJson =
                releases.optJSONObject(index);
            if (releaseJson == null
                || releaseJson.optBoolean("draft", false)) {
                continue;
            }

            Release release = parseRelease(releaseJson);
            if (!isReleaseNewer(
                    release,
                    currentVersion,
                    currentPhoneTestRun,
                    currentPhoneTestAttempt
                )) {
                continue;
            }

            if (release.apkUrl != null) {
                return new Result(
                    Result.State.UPDATE_AVAILABLE,
                    release,
                    "ReverieVR "
                        + release.version
                        + " is available."
                );
            }

            if (newerWithoutApk == null) {
                newerWithoutApk = release;
            }
        }

        if (newerWithoutApk != null) {
            return new Result(
                Result.State.RELEASE_WITHOUT_APK,
                newerWithoutApk,
                "A newer release exists, but it does not contain an APK asset."
            );
        }

        return new Result(
            Result.State.UP_TO_DATE,
            null,
            "Installed version is current."
        );
    }

    private static Release parseRelease(
        JSONObject releaseJson
    ) {
        String tagName =
            releaseJson.optString("tag_name", "");
        String title =
            releaseJson.optString("name", tagName);
        String notes =
            releaseJson.optString("body", "");
        String releasePage =
            releaseJson.optString("html_url", "");

        String apkUrl = null;
        String apkDigest = null;
        JSONArray assets =
            releaseJson.optJSONArray("assets");

        if (assets != null) {
            for (int index = 0;
                 index < assets.length();
                 index++) {
                JSONObject asset =
                    assets.optJSONObject(index);
                if (asset == null) {
                    continue;
                }

                String name =
                    asset.optString("name", "");
                String browserDownloadUrl =
                    asset.optString(
                        "browser_download_url",
                        ""
                    );
                if (isHeadsetApkAssetName(name)
                    && isTrustedReleaseAssetUrl(
                        browserDownloadUrl
                    )) {
                    apkUrl = browserDownloadUrl;
                    apkDigest =
                        asset.optString("digest", "");
                    break;
                }
            }
        }

        PhoneTestTag phoneTest =
            PhoneTestTag.parse(tagName);

        return new Release(
            tagName,
            title,
            notes,
            releasePage,
            apkUrl,
            apkDigest,
            phoneTest == null ? 0 : phoneTest.run,
            phoneTest == null ? 0 : phoneTest.attempt
        );
    }

    private static boolean isReleaseNewer(
        Release release,
        String currentVersion,
        int currentPhoneTestRun,
        int currentPhoneTestAttempt
    ) {
        if (release.phoneTestRun > 0) {
            return isPhoneTestNewer(
                release.phoneTestRun,
                release.phoneTestAttempt,
                currentPhoneTestRun,
                currentPhoneTestAttempt
            );
        }

        return VersionUtils.isNewer(
            release.version,
            currentVersion
        );
    }

    static boolean isPhoneTestNewer(
        int candidateRun,
        int candidateAttempt,
        int currentRun,
        int currentAttempt
    ) {
        if (currentRun <= 0 || candidateRun <= 0) {
            return false;
        }

        if (candidateRun != currentRun) {
            return candidateRun > currentRun;
        }

        return candidateAttempt > currentAttempt;
    }

    static boolean isHeadsetApkAssetName(String value) {
        if (value == null) {
            return false;
        }

        String normalized =
            value.trim().toLowerCase();
        if (!normalized.endsWith(".apk")) {
            return false;
        }

        if (normalized.contains("controller")) {
            return false;
        }

        return normalized.startsWith("reverievr-")
            || normalized.equals("reverievr.apk");
    }

    private static boolean isTrustedReleaseAssetUrl(
        String value
    ) {
        try {
            URL url = new URL(value);
            return "https".equalsIgnoreCase(
                    url.getProtocol()
                )
                && "github.com".equalsIgnoreCase(
                    url.getHost()
                )
                && url.getPath().startsWith(
                    "/mrcalzon02/ReverieVR/releases/download/"
                );
        } catch (Exception exception) {
            return false;
        }
    }

    private static String readAll(InputStream stream)
        throws IOException {
        StringBuilder builder =
            new StringBuilder();
        try (
            BufferedReader reader =
                new BufferedReader(
                    new InputStreamReader(
                        stream,
                        StandardCharsets.UTF_8
                    )
                )
        ) {
            String line;
            while ((line = reader.readLine()) != null) {
                builder.append(line).append('\n');
            }
        }
        return builder.toString();
    }

    private static String safeMessage(
        Exception exception
    ) {
        String message = exception.getMessage();
        return message == null
                || message.trim().isEmpty()
            ? "unknown error"
            : message;
    }

    @Override
    public void close() {
        executor.shutdownNow();
    }
}
