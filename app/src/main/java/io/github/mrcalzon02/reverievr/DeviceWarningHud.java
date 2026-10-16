package io.github.mrcalzon02.reverievr;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/** One-line optional phone thermal/power warning, independently layered in both eyes. */
final class DeviceWarningHud {
    private static final int WIDTH = 512;
    private static final int HEIGHT = 72;
    private static final float[] IDENTITY = {
        1f,0f,0f,0f, 0f,1f,0f,0f, 0f,0f,1f,0f, 0f,0f,0f,1f
    };
    private final FloatBuffer vertices = buffer(new float[] {
        -0.55f,-0.73f,0f, 0.55f,-0.73f,0f,
        -0.55f,-0.56f,0f, 0.55f,-0.56f,0f
    });
    private final FloatBuffer uvs = buffer(new float[] {
        0f,1f, 1f,1f, 0f,0f, 1f,0f
    });
    private Bitmap bitmap;
    private Canvas canvas;
    private Paint paint;
    private int texture;
    private boolean textureAllocated;
    private boolean dirty = true;
    private int level;

    void setLevel(int latest) {
        if (level == latest) return;
        level = latest;
        dirty = true;
    }

    void onSurfaceCreated() {
        int[] handles = new int[1];
        GLES20.glGenTextures(1, handles, 0);
        texture = handles[0];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        textureAllocated = false;
        dirty = true;
    }

    void draw(int program, int position, int uv, int matrix, int sampler) {
        if (program == 0 || texture == 0 || level == DeviceWarningPolicy.NONE) return;
        if (dirty) upload();
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glUseProgram(program);
        vertices.position(0);
        GLES20.glVertexAttribPointer(position, 3,
            GLES20.GL_FLOAT, false, 0, vertices);
        GLES20.glEnableVertexAttribArray(position);
        uvs.position(0);
        GLES20.glVertexAttribPointer(uv, 2,
            GLES20.GL_FLOAT, false, 0, uvs);
        GLES20.glEnableVertexAttribArray(uv);
        GLES20.glUniformMatrix4fv(matrix, 1, false, IDENTITY, 0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glUniform1i(sampler, 0);
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA,
            GLES20.GL_ONE_MINUS_SRC_ALPHA);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glDisableVertexAttribArray(position);
        GLES20.glDisableVertexAttribArray(uv);
    }

    private void upload() {
        if (bitmap == null || bitmap.isRecycled()) {
            bitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);
            canvas = new Canvas(bitmap);
            paint = new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setTypeface(Typeface.create(Typeface.MONOSPACE, Typeface.BOLD));
        }
        canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.argb(226, 25, 21, 17));
        canvas.drawRoundRect(0f,0f,WIDTH,HEIGHT,11f,11f,paint);
        paint.setColor(level >= DeviceWarningPolicy.BATTERY_HOT
            ? Color.rgb(255, 150, 83) : Color.rgb(255, 217, 107));
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(20f);
        canvas.drawText(DeviceWarningPolicy.text(level), WIDTH / 2f, 46f, paint);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        if (textureAllocated) {
            GLUtils.texSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, bitmap);
        } else {
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
            textureAllocated = true;
        }
        dirty = false;
    }

    void shutdown() {
        if (texture != 0) {
            GLES20.glDeleteTextures(1, new int[] {texture}, 0);
            texture = 0;
        }
        textureAllocated = false;
        dirty = true;
        if (bitmap != null && !bitmap.isRecycled()) bitmap.recycle();
        bitmap = null;
        canvas = null;
        paint = null;
    }

    private static FloatBuffer buffer(float[] data) {
        FloatBuffer result = ByteBuffer.allocateDirect(data.length * 4)
            .order(ByteOrder.nativeOrder()).asFloatBuffer();
        result.put(data);
        result.position(0);
        return result;
    }
}
