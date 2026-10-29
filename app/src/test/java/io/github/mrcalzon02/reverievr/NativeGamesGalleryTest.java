package io.github.mrcalzon02.reverievr;
import static org.junit.Assert.*;
import org.junit.Test;
public class NativeGamesGalleryTest {
    @Test public void fifteenRegisteredGamesHaveUniqueReachablePageSlots() {
        assertEquals(15, GameModuleRegistry.GAMES.length);
        assertEquals(3, NativeGamesGallery.pageCount(15));
        for(int i=0;i<15;i++)
            assertEquals(i,NativeGamesGallery.moduleIndex(i/6,i%6,15));
        assertEquals(-1,NativeGamesGallery.moduleIndex(2,3,15));
        assertEquals(-1,NativeGamesGallery.moduleIndex(3,0,15));
        assertEquals(-1,NativeGamesGallery.moduleIndex(-1,0,15));
        assertEquals(1,NativeGamesGallery.pageCount(0));
    }
    @Test public void originalAndClassicTitlesShareOneGallery() {
        int featured=0, classics=0;
        for(GameModuleRegistry.Entry item:GameModuleRegistry.GAMES) {
            if(item.id.startsWith("classic-")) {
                classics++;
                assertTrue(NativeGamesGallery.order(item.id)>=100);
            } else {
                featured++;
                assertTrue(NativeGamesGallery.order(item.id)<100);
            }
        }
        assertEquals(7,featured);
        assertEquals(8,classics);
    }
    @Test public void homeRoomGalleryKeepsAllGamesAndPhysicalPanelBounds() {
        assertEquals(4,NativeGamesGallery.homePageCount(15));
        for(int i=0;i<15;i++)
            assertEquals(i,NativeGamesGallery.homeModuleIndex(i/4,i%4,15));
        assertEquals(-1,NativeGamesGallery.homeModuleIndex(3,3,15));
        assertEquals(-1,NativeGamesGallery.homeModuleIndex(4,0,15));
        assertTrue(NativeGamesGallery.homeButtonEnabled(6,3,15));
        assertTrue(NativeGamesGallery.homeButtonEnabled(7,3,15));
        assertFalse(NativeGamesGallery.homeButtonEnabled(3,3,15));
        for(int i=0;i<NativeGamesGallery.HOME_BUTTONS.length;i++) {
            int[] a=NativeGamesGallery.HOME_BUTTONS[i];
            assertTrue(a[0]>=266 && a[2]<=744 && a[1]>=60 && a[3]<=710);
            assertEquals(i,NativeGamesGallery.homeButtonAtPixel(
                (a[0]+a[2])/2f,(a[1]+a[3])/2f));
            for(int j=i+1;j<NativeGamesGallery.HOME_BUTTONS.length;j++){
                int[] b=NativeGamesGallery.HOME_BUTTONS[j];
                assertFalse(a[0]<b[2]&&b[0]<a[2]&&a[1]<b[3]&&b[1]<a[3]);
            }
        }
        assertEquals(-1,NativeGamesGallery.homeButtonAtPixel(100,300));
    }
    @Test public void cardAndNavigationTargetsNeverOverlap() {
        assertEquals(10,NativeGamesGallery.BUTTONS.length);
        assertEquals(8,NativeGamesGallery.TOOLS);
        for(int i=0;i<NativeGamesGallery.BUTTONS.length;i++) {
            int[] a=NativeGamesGallery.BUTTONS[i];
            assertTrue(a[0]>=0&&a[1]>=0&&a[2]<=1024&&a[3]<=768);
            assertTrue(a[0]<a[2]&&a[1]<a[3]);
            for(int j=i+1;j<NativeGamesGallery.BUTTONS.length;j++) {
                int[] b=NativeGamesGallery.BUTTONS[j];
                assertFalse("overlap "+i+" "+j, a[0]<b[2]&&b[0]<a[2]
                    &&a[1]<b[3]&&b[1]<a[3]);
            }
        }
    }
}
