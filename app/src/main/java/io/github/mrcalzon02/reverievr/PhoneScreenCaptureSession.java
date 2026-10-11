package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.WindowManager;

/**
 * Explicitly consented capture lifecycle. This owns frames and resources;
 * it does not imply that a live GL panel or OS touch injection exists.
 */
final class PhoneScreenCaptureSession implements AutoCloseable {
    private MediaProjection projection;
    private VirtualDisplay display;
    private ImageReader frames;
    private volatile long receivedFrames;
    private final MediaProjection.Callback revoked = new MediaProjection.Callback() {
        @Override public void onStop() { releaseResources(); }
    };

    static Intent consentIntent(Activity activity) {
        MediaProjectionManager manager = (MediaProjectionManager)
            activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        return manager == null ? null : manager.createScreenCaptureIntent();
    }

    synchronized boolean start(Activity activity, int resultCode, Intent grant) {
        close();
        if (resultCode != Activity.RESULT_OK || grant == null) return false;
        MediaProjectionManager manager = (MediaProjectionManager)
            activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        if (manager == null) return false;
        try {
            projection = manager.getMediaProjection(resultCode, grant);
            if (projection == null) return false;
            projection.registerCallback(revoked, null);
            DisplayMetrics metrics = new DisplayMetrics();
            WindowManager window = (WindowManager)
                activity.getSystemService(Context.WINDOW_SERVICE);
            if (window == null) { close(); return false; }
            window.getDefaultDisplay().getRealMetrics(metrics);
            int width = metrics.widthPixels, height = metrics.heightPixels;
            if (width <= 0 || height <= 0) { close(); return false; }
            frames = ImageReader.newInstance(width, height,
                PixelFormat.RGBA_8888, 2);
            frames.setOnImageAvailableListener(reader -> {
                Image image = null;
                try {
                    image = reader.acquireLatestImage();
                    if (image != null) receivedFrames++;
                } catch (IllegalStateException stopped) {
                    // Projection revoked or reader closed between callbacks.
                } finally {
                    if (image != null) image.close();
                }
            }, null);
            display = projection.createVirtualDisplay("ReverieVR Phone Screen",
                width, height, metrics.densityDpi,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                frames.getSurface(), null, null);
            if (display == null) { close(); return false; }
            return true;
        } catch (RuntimeException failure) {
            close();
            ReverieLog.error("PHONE_CAPTURE", "Capture setup failed", failure);
            return false;
        }
    }

    synchronized boolean isActive() {
        return projection != null && display != null;
    }

    synchronized long frameCount() { return receivedFrames; }

    private synchronized void releaseResources() {
        if (display != null) { display.release(); display = null; }
        if (frames != null) { frames.close(); frames = null; }
    }

    @Override public synchronized void close() {
        releaseResources();
        if (projection != null) {
            MediaProjection previous = projection;
            projection = null;
            try { previous.unregisterCallback(revoked); }
            catch (RuntimeException ignored) { }
            previous.stop();
        }
    }
}
