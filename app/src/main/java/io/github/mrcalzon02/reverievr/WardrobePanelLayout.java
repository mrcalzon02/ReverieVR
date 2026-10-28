package io.github.mrcalzon02.reverievr;

/**
 * Fixed 1024x768 wardrobe texture geometry, shared by rendering and ray hit tests.
 * Each field label occupies the left column; its Previous/Next targets are on
 * the same row, never underneath the label. Keep hit bounds and draw bounds one
 * source of truth so controls remain aligned in both eyes.
 */
final class WardrobePanelLayout {
    private WardrobePanelLayout() {}

    static final int FIELD_LABEL_LEFT = 125;
    static final int FIELD_LABEL_RIGHT = 520;
    static final int PAGE_LABEL_BASELINE = 181;

    // Indices 0-9: five Previous/Next pairs. 10-12: actions. 13-14: pages.
    static final int[][] BUTTONS = {
        {545,251,710,301}, {726,251,904,301},
        {545,325,710,375}, {726,325,904,375},
        {545,399,710,449}, {726,399,904,449},
        {545,473,710,523}, {726,473,904,523},
        {545,547,710,597}, {726,547,904,597},
        {120,635,350,695}, {370,635,650,695}, {670,635,904,695},
        {120,194,430,236}, {452,194,754,236}
    };

    static float fieldLabelBaseline(int row, float fontAscent, float fontDescent) {
        int[] rect = BUTTONS[row * 2];
        return (rect[1] + rect[3]) * .5f - (fontAscent + fontDescent) * .5f;
    }
}
