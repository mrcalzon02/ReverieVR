package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicLong;

final class ReverieLog {
    private static final String LOGCAT_TAG = "ReverieVR";

    private static final long STANDARD_MAX_BYTES =
        1L * 1024L * 1024L;
    private static final long DEVELOPMENT_MAX_BYTES =
        16L * 1024L * 1024L;
    private static final int STANDARD_ARCHIVES = 4;
    private static final int DEVELOPMENT_ARCHIVES = 4;

    private static final LinkedBlockingQueue<Record> QUEUE =
        new LinkedBlockingQueue<>(8192);
    private static final AtomicLong SEQUENCE =
        new AtomicLong();

    private static volatile LoggingMode mode =
        LoggingMode.STANDARD;
    private static volatile boolean initialized;
    private static volatile boolean running;
    private static File logDirectory;
    private static Thread writerThread;

    private ReverieLog() {
    }

    static synchronized void initialize(
        Context context,
        LoggingMode requestedMode
    ) {
        mode = requestedMode == null
            ? LoggingMode.STANDARD
            : requestedMode;

        if (initialized) {
            return;
        }

        Context appContext =
            context.getApplicationContext();
        logDirectory =
            new File(appContext.getFilesDir(), "logs");

        if (!logDirectory.isDirectory()
            && !logDirectory.mkdirs()) {
            Log.e(
                LOGCAT_TAG,
                "Could not create private log directory."
            );
        }

        running = true;
        writerThread = new Thread(
            ReverieLog::writerLoop,
            "ReverieLogWriter"
        );
        writerThread.setDaemon(true);
        writerThread.start();
        initialized = true;

        milestone(
            "LOG",
            "Logging initialized in "
                + mode.name()
                + " mode."
        );
    }

    static LoggingMode getMode() {
        return mode;
    }

    static boolean isDevelopment() {
        return mode == LoggingMode.DEVELOPMENT;
    }

    static void setMode(LoggingMode newMode) {
        LoggingMode safe =
            newMode == null
                ? LoggingMode.STANDARD
                : newMode;

        LoggingMode previous = mode;
        mode = safe;

        if (previous != safe) {
            milestone(
                "LOG",
                "Logging mode changed from "
                    + previous.name()
                    + " to "
                    + safe.name()
                    + "."
            );
        }
    }

    static void milestone(String area, String message) {
        enqueue(
            "MILESTONE",
            area,
            message,
            false,
            null
        );
        Log.i(
            LOGCAT_TAG,
            compact(area, message)
        );
    }

    static void incident(String area, String message) {
        enqueue(
            "INCIDENT",
            area,
            message,
            false,
            null
        );
        Log.w(
            LOGCAT_TAG,
            compact(area, message)
        );
    }

    static void error(
        String area,
        String message,
        Throwable throwable
    ) {
        enqueue(
            "ERROR",
            area,
            message,
            false,
            throwable
        );
        Log.e(
            LOGCAT_TAG,
            compact(area, message),
            throwable
        );
    }

    static void fatal(
        String area,
        String message,
        Throwable throwable
    ) {
        enqueue(
            "FATAL",
            area,
            message,
            false,
            throwable
        );

        List<Record> urgent = new ArrayList<>();
        QUEUE.drainTo(urgent);
        if (!urgent.isEmpty()) {
            writeBatch(urgent);
        }

        Log.e(
            LOGCAT_TAG,
            compact(area, message),
            throwable
        );
    }

    static void dev(String area, String message) {
        if (!isDevelopment()) {
            return;
        }

        enqueue(
            "DEV",
            area,
            message,
            true,
            null
        );
        Log.v(
            LOGCAT_TAG,
            compact(area, message)
        );
    }

    static String getLogDirectoryPath() {
        File directory = logDirectory;
        return directory == null
            ? ""
            : directory.getAbsolutePath();
    }

    static synchronized void clearLogs() {
        QUEUE.clear();

        File directory = logDirectory;
        if (directory == null || !directory.isDirectory()) {
            return;
        }

        File[] files = directory.listFiles();
        if (files == null) {
            return;
        }

        for (File file : files) {
            if (file.isFile()
                && file.getName().startsWith("reverie-")) {
                if (!file.delete()) {
                    Log.w(
                        LOGCAT_TAG,
                        "Could not delete log file "
                            + file.getAbsolutePath()
                    );
                }
            }
        }

        milestone("LOG", "Private log history cleared.");
    }

    static synchronized void shutdown() {
        if (!initialized) {
            return;
        }

        milestone("APP", "Logger shutdown requested.");
        running = false;

        Thread thread = writerThread;
        if (thread != null) {
            thread.interrupt();
        }

        initialized = false;
        writerThread = null;
    }

    private static void enqueue(
        String level,
        String area,
        String message,
        boolean developmentOnly,
        Throwable throwable
    ) {
        if (!initialized) {
            return;
        }

        String safeArea =
            area == null || area.trim().isEmpty()
                ? "GENERAL"
                : area.trim();
        String safeMessage =
            message == null
                ? ""
                : message.replace('\n', ' ')
                    .replace('\r', ' ');

        StringBuilder line = new StringBuilder(256);
        line.append(Instant.now())
            .append(" #")
            .append(SEQUENCE.incrementAndGet())
            .append(" [")
            .append(level)
            .append("] [")
            .append(safeArea)
            .append("] [")
            .append(Thread.currentThread().getName())
            .append("] ")
            .append(safeMessage);

        if (throwable != null) {
            line.append(" | ")
                .append(throwable.getClass().getName());

            String throwableMessage = throwable.getMessage();
            if (throwableMessage != null
                && !throwableMessage.trim().isEmpty()) {
                line.append(": ")
                    .append(
                        throwableMessage
                            .replace('\n', ' ')
                            .replace('\r', ' ')
                    );
            }
        }

        Record record = new Record(
            line.append('\n').toString(),
            developmentOnly,
            mode == LoggingMode.DEVELOPMENT
        );

        try {
            // Development logging is deliberately lossless and may block.
            // Diagnostic runs are not performance-acceptance runs.
            QUEUE.put(record);
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            Log.e(
                LOGCAT_TAG,
                "Interrupted while queueing a log record.",
                exception
            );
        }
    }

    private static void writerLoop() {
        List<Record> batch = new ArrayList<>(256);

        while (running || !QUEUE.isEmpty()) {
            try {
                Record first = QUEUE.take();
                batch.add(first);
                QUEUE.drainTo(batch, 255);

                writeBatch(batch);
                batch.clear();
            } catch (InterruptedException exception) {
                if (!running) {
                    drainRemaining(batch);
                    return;
                }
            } catch (RuntimeException exception) {
                Log.e(
                    LOGCAT_TAG,
                    "Unexpected log-writer failure.",
                    exception
                );
                batch.clear();
            }
        }
    }

    private static void drainRemaining(List<Record> batch) {
        QUEUE.drainTo(batch);
        if (!batch.isEmpty()) {
            writeBatch(batch);
            batch.clear();
        }
    }

    private static void writeBatch(List<Record> batch) {
        if (batch.isEmpty() || logDirectory == null) {
            return;
        }

        StringBuilder standard = new StringBuilder();
        StringBuilder development = new StringBuilder();

        for (Record record : batch) {
            if (!record.developmentOnly) {
                standard.append(record.line);
            }
            if (record.developmentModeAtWrite) {
                development.append(record.line);
            }
        }

        if (standard.length() > 0) {
            append(
                new File(
                    logDirectory,
                    "reverie-standard.log"
                ),
                standard.toString(),
                STANDARD_MAX_BYTES,
                STANDARD_ARCHIVES
            );
        }

        if (development.length() > 0) {
            append(
                new File(
                    logDirectory,
                    "reverie-development.log"
                ),
                development.toString(),
                DEVELOPMENT_MAX_BYTES,
                DEVELOPMENT_ARCHIVES
            );
        }
    }

    private static void append(
        File file,
        String value,
        long maxBytes,
        int archives
    ) {
        byte[] bytes =
            value.getBytes(StandardCharsets.UTF_8);

        if (file.length() + bytes.length > maxBytes) {
            rotate(file, archives);
        }

        try (BufferedOutputStream output =
                 new BufferedOutputStream(
                     new FileOutputStream(file, true)
                 )) {
            output.write(bytes);
        } catch (IOException exception) {
            Log.e(
                LOGCAT_TAG,
                "Could not write "
                    + file.getAbsolutePath(),
                exception
            );
        }
    }

    private static void rotate(File file, int archives) {
        for (int index = archives; index >= 1; index--) {
            File source = index == 1
                ? file
                : new File(
                    file.getParentFile(),
                    file.getName()
                        + "."
                        + (index - 1)
                );
            File destination =
                new File(
                    file.getParentFile(),
                    file.getName()
                        + "."
                        + index
                );

            if (!source.exists()) {
                continue;
            }

            if (destination.exists()
                && !destination.delete()) {
                Log.w(
                    LOGCAT_TAG,
                    "Could not delete rotated log "
                        + destination.getAbsolutePath()
                );
            }

            if (!source.renameTo(destination)) {
                Log.w(
                    LOGCAT_TAG,
                    "Could not rotate log "
                        + source.getAbsolutePath()
                );
            }
        }
    }

    private static String compact(
        String area,
        String message
    ) {
        return "["
            + (area == null ? "GENERAL" : area)
            + "] "
            + (message == null ? "" : message);
    }

    private static final class Record {
        final String line;
        final boolean developmentOnly;
        final boolean developmentModeAtWrite;

        Record(
            String line,
            boolean developmentOnly,
            boolean developmentModeAtWrite
        ) {
            this.line = line;
            this.developmentOnly = developmentOnly;
            this.developmentModeAtWrite =
                developmentModeAtWrite;
        }
    }
}
