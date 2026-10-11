package io.github.mrcalzon02.reverievr;

/** Fixed normal for untextured small cap materials; not the shaded housing. */
final class DaydreamControllerSurfaceLighting {
    private DaydreamControllerSurfaceLighting() {}

    static boolean flatButtonNormal(String material) {
        return "MatAppButton".equals(material)
            || "MatHomeButton".equals(material)
            || "MatVolUpBut".equals(material)
            || "MatVolDownBut".equals(material);
    }
}
