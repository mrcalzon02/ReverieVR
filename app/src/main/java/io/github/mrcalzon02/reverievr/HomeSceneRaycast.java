package io.github.mrcalzon02.reverievr;

/**
 * Immutable generated triangle mesh + allocation-free nearest surface test.
 * Built once with each Home geometry buffer; one ray test per input frame,
 * NOT per stereo eye and never by reading GPU pixels or depth buffers.
 */
final class HomeSceneRaycast {
    static final float MAX_DISTANCE = 12f;
    private static final float EPSILON = 0.000001f;

    static final class Mesh {
        final float[] xyz;
        final float[] min = {Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY,
            Float.POSITIVE_INFINITY};
        final float[] max = {Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY,
            Float.NEGATIVE_INFINITY};

        Mesh(float[] xyz) {
            if (xyz == null || xyz.length % 9 != 0)
                throw new IllegalArgumentException("Triangle mesh requires complete triangles");
            this.xyz = xyz;
            for (int i = 0; i < xyz.length; i += 3) {
                for (int a = 0; a < 3; a++) {
                    float v = xyz[i + a];
                    if (!Float.isFinite(v)) throw new IllegalArgumentException("nonfinite mesh");
                    min[a] = Math.min(min[a], v);
                    max[a] = Math.max(max[a], v);
                }
            }
        }
    }

    static final class Hit {
        float distance = Float.POSITIVE_INFINITY;
        float normalX;
        float normalY;
        float normalZ;
        void reset() {
            distance = Float.POSITIVE_INFINITY;
            normalX = normalY = normalZ = 0f;
        }
        boolean hasContact() {
            return Float.isFinite(distance) && distance > 0f;
        }
    }

    private HomeSceneRaycast() {}

    static void test(Mesh mesh, float[] origin, float[] direction, Hit result) {
        if (mesh == null || mesh.xyz.length == 0 || result == null
            || origin == null || direction == null
            || origin.length < 3 || direction.length < 3) return;
        for (int a = 0; a < 3; a++) {
            if (!Float.isFinite(origin[a]) || !Float.isFinite(direction[a])) return;
        }
        if (!boundsIntersect(mesh, origin, direction,
                Math.min(MAX_DISTANCE, result.distance))) return;
        float[] v = mesh.xyz;
        for (int i = 0; i < v.length; i += 9) {
            float ax = v[i], ay = v[i+1], az = v[i+2];
            float e1x = v[i+3]-ax, e1y = v[i+4]-ay, e1z = v[i+5]-az;
            float e2x = v[i+6]-ax, e2y = v[i+7]-ay, e2z = v[i+8]-az;
            float px = direction[1]*e2z-direction[2]*e2y;
            float py = direction[2]*e2x-direction[0]*e2z;
            float pz = direction[0]*e2y-direction[1]*e2x;
            float det = e1x*px + e1y*py + e1z*pz;
            if (Math.abs(det) < EPSILON) continue;
            float inv = 1f/det;
            float tx = origin[0]-ax, ty = origin[1]-ay, tz = origin[2]-az;
            float u = (tx*px+ty*py+tz*pz)*inv;
            if (u < 0f || u > 1f) continue;
            float qx = ty*e1z-tz*e1y;
            float qy = tz*e1x-tx*e1z;
            float qz = tx*e1y-ty*e1x;
            float w = (direction[0]*qx+direction[1]*qy+direction[2]*qz)*inv;
            if (w < 0f || u+w > 1f) continue;
            float t = (e2x*qx+e2y*qy+e2z*qz)*inv;
            if (!Float.isFinite(t) || t <= EPSILON
                || t >= result.distance || t > MAX_DISTANCE) continue;
            float nx = e1y*e2z-e1z*e2y;
            float ny = e1z*e2x-e1x*e2z;
            float nz = e1x*e2y-e1y*e2x;
            float n2 = nx*nx+ny*ny+nz*nz;
            if (n2 <= EPSILON*EPSILON || !Float.isFinite(n2)) continue;
            float inverseLength = 1f/(float)Math.sqrt(n2);
            // Orient visible target decal toward the incoming beam, regardless
            // of mesh winding, so it lies just outside the solid surface.
            if (nx*direction[0]+ny*direction[1]+nz*direction[2] > 0f)
                inverseLength = -inverseLength;
            result.distance = t;
            result.normalX = nx*inverseLength;
            result.normalY = ny*inverseLength;
            result.normalZ = nz*inverseLength;
        }
    }

    private static boolean boundsIntersect(Mesh mesh, float[] o, float[] d,
                                           float maximumDistance) {
        float near = 0f, far = maximumDistance;
        for (int a=0; a<3; a++) {
            if (Math.abs(d[a]) < EPSILON) {
                if (o[a] < mesh.min[a] || o[a] > mesh.max[a]) return false;
                continue;
            }
            float t0=(mesh.min[a]-o[a])/d[a];
            float t1=(mesh.max[a]-o[a])/d[a];
            if (t0>t1) { float temp=t0; t0=t1; t1=temp; }
            near=Math.max(near,t0);
            far=Math.min(far,t1);
            if (far<near) return false;
        }
        return far>0f;
    }
}
