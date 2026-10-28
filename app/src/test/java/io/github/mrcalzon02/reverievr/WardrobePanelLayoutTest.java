package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

/** Geometry checks for the stereo wardrobe menu and controller/gaze hit areas. */
public class WardrobePanelLayoutTest {
    @Test public void fiveRowsHaveIndependentLabelAndButtonColumns() {
        assertEquals(15, WardrobePanelLayout.BUTTONS.length);
        for (int row=0; row<5; row++) {
            int[] prev=WardrobePanelLayout.BUTTONS[row*2];
            int[] next=WardrobePanelLayout.BUTTONS[row*2+1];
            assertTrue("label must not intersect Prev: row "+row,
                WardrobePanelLayout.FIELD_LABEL_RIGHT+15 <= prev[0]);
            assertEquals(prev[1],next[1]);
            assertEquals(prev[3],next[3]);
            assertTrue(prev[2]+12 <= next[0]);
            assertTrue(prev[2]-prev[0] >= 160);
            assertTrue(next[2]-next[0] >= 160);
            assertEquals(50,prev[3]-prev[1]);
            float baseline=WardrobePanelLayout.fieldLabelBaseline(row,-22f,5f);
            assertTrue(baseline > prev[1]+15 && baseline < prev[3]-7);
            if(row>0)assertTrue(prev[1]-WardrobePanelLayout.BUTTONS[(row-1)*2][3]>=20);
        }
    }

    @Test public void headerPageNavigationAndActionsStaySeparate() {
        int[] pageBack=WardrobePanelLayout.BUTTONS[13];
        int[] pageNext=WardrobePanelLayout.BUTTONS[14];
        assertTrue(pageBack[1]>WardrobePanelLayout.PAGE_LABEL_BASELINE+8);
        assertEquals(pageBack[1],pageNext[1]);
        assertEquals(pageBack[3],pageNext[3]);
        assertTrue(pageBack[2]+12<=pageNext[0]);
        assertTrue(WardrobePanelLayout.BUTTONS[0][1]>pageBack[3]+10);
        for(int index=10;index<=12;index++) {
            int[] action=WardrobePanelLayout.BUTTONS[index];
            assertTrue(action[1]>WardrobePanelLayout.BUTTONS[8][3]+25);
            assertTrue(action[3]<=726);
        }
    }

    @Test public void everyHitTargetIsDistinctAndWithinTexture() {
        for(int i=0;i<WardrobePanelLayout.BUTTONS.length;i++) {
            int[] a=WardrobePanelLayout.BUTTONS[i];
            assertTrue(a[0]>=0 && a[1]>=0 && a[2]<=1024 && a[3]<=768);
            assertTrue(a[0]<a[2] && a[1]<a[3]);
            for(int j=i+1;j<WardrobePanelLayout.BUTTONS.length;j++) {
                int[] b=WardrobePanelLayout.BUTTONS[j];
                assertFalse("Targets overlap: "+i+" and "+j,
                    a[0]<b[2] && b[0]<a[2] && a[1]<b[3] && b[1]<a[3]);
            }
        }
    }
}
