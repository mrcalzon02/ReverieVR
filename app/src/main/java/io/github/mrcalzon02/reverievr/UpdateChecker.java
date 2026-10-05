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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

final class UpdateChecker implements AutoCloseable {
    static final String RELEASES_API =
        "https://api.github.com/repos/mrcalzon02/ReverieVR/releases/latest";

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

        Release(
            String version,
            String title,
            String notes,
            String releasePageUrl,
            String apkUrl,
            String apkDigest
        ) {
            this.version = version;
            this.title = title;
            this.notes = notes;
            this.releasePageUrl = releasePageUrl;
            this.apkUrl = apkUrl;
            this.apkDigest = apkDigest;
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

    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    void check(String currentVersion, Callback callback) {
        executor.execute(() -> callback.onResult(checkBlocking(currentVersion)));
    }

    private Result checkBlocking(String currentVersion) {
        HttpURLConnection connection = null;

        try {
            URL endpoint = new URL(RELEASES_API);
            connection = (HttpURLConnection) endpoint.openConnection();
            connection.setConnectTimeout(8000);
            connection.setReadTimeout(8000);
            connection.setRequestProperty("Accept", "application/vnd.github+json");
            connection.setRequestProperty("X-GitHub-Api-Version", "2022-11-28");
            connection.setRequestProperty("User-Agent", "ReverieVR/" + currentVersion);

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

            String json = readAll(connection.getInputStream());
            JSONObject releaseJson = new JSONObject(json);

            String tagName = releaseJson.optString("tag_name", "");
            String title = releaseJson.optString("name", tagName);
            String notes = releaseJson.optString("body", "");
            String releasePage = releaseJson.optString("html_url", "");

            String apkUrl = null;
            String apkDigest = null;
            JSONArray assets = releaseJson.optJSONArray("assets");

            if (assets != null) {
                for (int index = 0; index < assets.length(); index++) {
                    JSONObject asset = assets.optJSONObject(index);
                    if (asset == null) {
                        continue;
                    }

                    String name = asset.optString("name", "");
                    String browserDownloadUrl = asset.optString("browser_download_url", "");
                    if (isHeadsetApkAssetName(name)
                        && isTrustedReleaseAssetUrl(browserDownloadUrl)) {
                        apkUrl = browserDownloadUrl;
                        apkDigest = asset.optString("digest", "");
                        break;
                    }
                }
            }

            Release release = new Release(
                tagName,
                title,
                notes,
                releasePage,
                apkUrl,
                apkDigest
            );

            if (!VersionUtils.isNewer(tagName, currentVersion)) {
                return new Result(
                    Result.State.UP_TO_DATE,
                    release,
                    "Installed version is current."
                );
            }

            if (apkUrl == null) {
                return new Result(
                    Result.State.RELEASE_WITHOUT_APK,
                    release,
                    "A newer release exists, but it does not contain an APK asset."
                );
            }

            return new Result(
                Result.State.UPDATE_AVAILABLE,
                release,
                "ReverieVR " + tagName + " is available."
            );
        } catch (Exception exception) {
            return new Result(
                Result.State.ERROR,
                null,
                exception.getClass().getSimpleName() + ": " + safeMessage(exception)
            );
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
    }

    static boolean isHeadsetApkAssetName(String value) {
        if (value == null) {
            return false;
        }

        String normalized = value.trim().toLowerCase();
        if (!normalized.endsWith(".apk")) {
            return false;
        }

        if (normalized.contains("controller")) {
            return false;
        }

        return normalized.startsWith("reverievr-")
            || normalized.equals("reverievr.apk")
            || normalized.equals("app-release.apk")
            || normalized.equals("app-debug.apk");
    }

    private static boolean isTrustedReleaseAssetUrl(String value) {
        try {
            URL url = new URL(value);
            return "https".equalsIgnoreCase(url.getProtocol())
                && "github.com".equalsIgnoreCase(url.getHost())
                && url.getPath().startsWith("/mrcalzon02/ReverieVR/releases/download/");
        } catch (Exception exception) {
            return false;
        }
    }

    private static String readAll(InputStream stream) throws IOException {
        StringBuilder builder = new StringBuilder();
        try (
            BufferedReader reader = new BufferedReader(
                new InputStreamReader(stream, StandardCharsets.UTF_8)
            )
        ) {
            String line;
            while ((line = reader.readLine()) != null) {
                builder.append(line).append('\n');
            }
        }
        return builder.toString();
    }

    private static String safeMessage(Exception exception) {
        String message = exception.getMessage();
        return message == null || message.trim().isEmpty() ? "unknown error" : message;
    }

    @Override
    public void close() {
        executor.shutdownNow();
    }
}
