package io.github.mrcalzon02.reverievr;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Color;
import android.graphics.Paint;
import java.util.List;

/** Data-driven PSX gallery for actual loaded native modules, not static menu art. */
final class NativeGamesGallery {
    static final int PAGE_SIZE=6, PREVIOUS=6, NEXT=7, TOOLS=8, HOME=9;
    static final int[][] BUTTONS={
        {60,177,343,397},{370,177,653,397},{680,177,963,397},
        {60,416,343,636},{370,416,653,636},{680,416,963,636},
        {61,650,257,704},{268,650,464,704},{475,650,765,704},
        {776,650,963,704}
    };
    // The reference's actual seven-card 4+3 arrangement is drawn on the
    // physical Home wall. Other registered games follow on later pages.
    static final int HOME_PAGE_SIZE=7, HOME_PREVIOUS=7, HOME_NEXT=8,
        HOME_TOOLS=9, HOME_CLOSE=10;
    static final int ATLAS_CELL_WIDTH=84, ATLAS_CELL_HEIGHT=115;
    static final int[][] HOME_BUTTONS={
        {278,152,384,353},{393,152,499,353},
        {508,152,614,353},{623,152,730,353},
        {332,371,446,574},{453,371,567,574},{574,371,688,574},
        {278,617,384,693},{393,617,499,693},
        {508,617,614,693},{623,617,730,693}
    };
    static int homePageCount(int count) {
        return Math.max(1,(Math.max(0,count)+HOME_PAGE_SIZE-1)/HOME_PAGE_SIZE);
    }
    static int homeModuleIndex(int page,int slot,int count) {
        if(page<0||page>=homePageCount(count)||slot<0||slot>=HOME_PAGE_SIZE)
            return -1;
        int index=page*HOME_PAGE_SIZE+slot;
        return index<count?index:-1;
    }
    static boolean homeButtonEnabled(int slot,int page,int count) {
        if(slot>=0&&slot<HOME_PAGE_SIZE)
            return homeModuleIndex(page,slot,count)>=0;
        if(slot==HOME_PREVIOUS||slot==HOME_NEXT)
            return homePageCount(count)>1;
        return slot==HOME_TOOLS||slot==HOME_CLOSE;
    }
    static int homeButtonAtPixel(float x,float y) {
        for(int i=0;i<HOME_BUTTONS.length;i++) {
            int[] r=HOME_BUTTONS[i];
            if(x>=r[0]&&x<=r[2]&&y>=r[1]&&y<=r[3])return i;
        }
        return -1;
    }
    static void drawInHomeRoom(Canvas c,Paint p,
            List<NativeModuleRuntime.Descriptor> games,
            int requestedPage,int hover,float scale,String error,Bitmap atlas) {
        int pages=homePageCount(games.size());
        int page=Math.max(0,Math.min(requestedPage,pages-1));
        p.setStyle(Paint.Style.FILL);
        p.setColor(Color.rgb(8,17,28));
        c.drawRoundRect(268,61,742,708,10,10,p);
        p.setColor(Color.rgb(71,137,184));
        p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(2f);
        c.drawRoundRect(269,62,741,706,8,8,p);
        p.setStyle(Paint.Style.FILL);
        p.setColor(Color.rgb(216,238,247));
        fit(c,p,"REVERIEVR  /  NATIVE GAMES",278,100,445,
            26f*Math.min(scale,1.1f),false);
        p.setColor(error!=null&&!error.isEmpty()?Color.rgb(252,187,124)
            :Color.rgb(156,198,213));
        fit(c,p,error!=null&&!error.isEmpty()?"FAILED: "+error
            :games.size()+" INSTALLED / PAGE "+(page+1)+" OF "+pages,
            278,132,445,16f*Math.min(scale,1.1f),false);
        for(int slot=0;slot<HOME_PAGE_SIZE;slot++) {
            int index=homeModuleIndex(page,slot,games.size());
            if(index>=0)drawCard(c,p,HOME_BUTTONS[slot],games.get(index),
                hover==slot,scale,atlas);
            else empty(c,p,HOME_BUTTONS[slot]);
        }
        navAt(c,p,HOME_BUTTONS,HOME_PREVIOUS,"PREV",pages>1,hover,scale);
        navAt(c,p,HOME_BUTTONS,HOME_NEXT,"NEXT",pages>1,hover,scale);
        navAt(c,p,HOME_BUTTONS,HOME_TOOLS,"TOOLS",true,hover,scale);
        navAt(c,p,HOME_BUTTONS,HOME_CLOSE,"CLOSE",true,hover,scale);
        p.setTextAlign(Paint.Align.LEFT);p.setStyle(Paint.Style.FILL);
        p.setFakeBoldText(false);
    }
    private static final String[] FEATURED={
        "breakwater-battery","between-deliveries-red-ledger",
        "gumball-armada","iron-sight-forward-detachment",
        "ministry-lantern-desk","omega-front","procedural-test-chamber"
    };
    static int order(String id) {
        for(int i=0;i<FEATURED.length;i++)if(FEATURED[i].equals(id))return i;
        return 100; // Stable host registry order for remaining playable classics.
    }
    static int pageCount(int count) {
        return Math.max(1,(Math.max(0,count)+PAGE_SIZE-1)/PAGE_SIZE);
    }
    static int moduleIndex(int page,int slot,int count) {
        if(page<0||page>=pageCount(count)||slot<0||slot>=PAGE_SIZE)return -1;
        int index=page*PAGE_SIZE+slot;
        return index<count?index:-1;
    }
    static void draw(Canvas c,Paint p,List<NativeModuleRuntime.Descriptor> games,
                     int currentPage,int hover,float scale,boolean dev,String error,Bitmap atlas) {
        int pages=pageCount(games.size());
        int page=Math.max(0,Math.min(currentPage,pages-1));
        p.setStyle(Paint.Style.FILL);
        p.setTextAlign(Paint.Align.LEFT);
        p.setColor(Color.rgb(215,236,249));
        p.setFakeBoldText(true);
        p.setTextSize(35f*Math.min(scale,1.10f));
        c.drawText("REVERIEVR  NATIVE GAMES",61,104,p);
        p.setFakeBoldText(false);
        p.setColor(Color.rgb(79,143,183));
        p.setStrokeWidth(2f);
        c.drawLine(61,116,963,116,p);
        p.setColor(error!=null&&!error.isEmpty()?Color.rgb(245,184,118)
            :Color.rgb(160,192,206));
        fit(c,p,error!=null&&!error.isEmpty()?"LAUNCH FAILED: "+error
            :games.size()+" AVAILABLE  /  PAGE "+(page+1)+" OF "+pages
                +(dev?"  /  DEVELOPMENT MODE":""),
            61,146,900,17f*Math.min(scale,1.1f),false);
        for(int i=0;i<PAGE_SIZE;i++) {
            int index=moduleIndex(page,i,games.size());
            if(index>=0)drawCard(c,p,BUTTONS[i],games.get(index),hover==i,scale,atlas);
            else empty(c,p,BUTTONS[i]);
        }
        nav(c,p,PREVIOUS,"PREVIOUS",pages>1,hover,scale);
        nav(c,p,NEXT,"NEXT",pages>1,hover,scale);
        nav(c,p,TOOLS,"NATIVE TOOLS",true,hover,scale);
        nav(c,p,HOME,"HOME",true,hover,scale);
        p.setTextAlign(Paint.Align.LEFT);
        p.setStyle(Paint.Style.FILL);
        p.setFakeBoldText(false);
    }
    private static void empty(Canvas c,Paint p,int[] r) {
        p.setStyle(Paint.Style.FILL);p.setColor(Color.rgb(16,24,32));
        c.drawRoundRect(r[0],r[1],r[2],r[3],8,8,p);
        p.setColor(Color.rgb(40,59,75));p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(1.5f);
        c.drawRoundRect(r[0],r[1],r[2],r[3],8,8,p);
        p.setStyle(Paint.Style.FILL);
    }
    private static void drawCard(Canvas c,Paint p,int[] r,
             NativeModuleRuntime.Descriptor game,boolean hover,float scale,Bitmap atlas) {
        float x=r[0],y=r[1],right=r[2],bottom=r[3];
        p.setStyle(Paint.Style.FILL);
        p.setColor(Color.rgb(6,13,23));
        c.drawRoundRect(x,y,right,bottom,8,8,p);
        c.save();
        c.clipRect(x+7,y+7,right-7,bottom-60);
        preview(c,p,game.id,x+7,y+7,right-7,bottom-60,atlas);
        c.restore();
        p.setColor(Color.rgb(13,23,36));
        c.drawRect(x+5,bottom-59,right-5,bottom-4,p);
        p.setColor(Color.rgb(225,239,248));
        String[] lines=titleLines(game);
        if(lines.length==1) {
            fit(c,p,lines[0],(x+right)/2f,bottom-29,right-x-14,
                15f*Math.min(scale,1.12f),true);
        } else {
            fit(c,p,lines[0],(x+right)/2f,bottom-39,right-x-14,
                13f*Math.min(scale,1.12f),true);
            fit(c,p,lines[1],(x+right)/2f,bottom-24,right-x-14,
                13f*Math.min(scale,1.12f),true);
        }
        p.setColor(Color.rgb(137,190,201));
        fit(c,p,NativeModuleRuntime.isDevelopmentOnly(game.id)
            ?"DEV / PLAY":"SELECT TO PLAY",
            (x+right)/2f,bottom-9,right-x-14,10f,true);
        p.setColor(hover?Color.rgb(251,214,103):Color.rgb(65,131,183));
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(hover?4f:2f);
        c.drawRoundRect(x,y,right,bottom,8,8,p);
        p.setStyle(Paint.Style.FILL);
    }
    private static String[] titleLines(NativeModuleRuntime.Descriptor d) {
        switch(d.id) {
            case "between-deliveries-red-ledger":
                return new String[]{"Between Deliveries","Red Ledger"};
            case "iron-sight-forward-detachment":
                return new String[]{"Iron Sight","Forward Detachment"};
            case "ministry-lantern-desk":
                return new String[]{"Ministry","Lantern Desk"};
            case "procedural-test-chamber":
                return new String[]{"Procedural","Test Chamber"};
            default:return new String[]{title(d)};
        }
    }
    /** Atlas cells retain the seven separate game-menu pictures in the supplied
     * reference. Unlike a painted mockup, each cell is attached to a real id. */
    static int[] atlasCell(String id) {
        int index=order(id);
        if(index<0||index>=FEATURED.length)return null;
        return new int[]{(index%4)*ATLAS_CELL_WIDTH,
            (index/4)*ATLAS_CELL_HEIGHT};
    }
    private static String title(NativeModuleRuntime.Descriptor d) {
        switch(d.id) {
            case "breakwater-battery":return "Breakwater Battery";
            case "between-deliveries-red-ledger":return "Between Deliveries / Red Ledger";
            case "gumball-armada":return "Gumball Armada";
            case "iron-sight-forward-detachment":return "Iron Sight: Forward Detachment";
            case "ministry-lantern-desk":return "Ministry: Lantern Desk";
            case "omega-front":return "Omega Front";
            case "procedural-test-chamber":return "Procedural Test Chamber";
            default:return d.displayName.replace("[DEV]","").trim();
        }
    }
    private static void nav(Canvas c,Paint p,int index,String label,
                            boolean enabled,int hovered,float scale) {
        navAt(c,p,BUTTONS,index,label,enabled,hovered,scale);
    }
    private static void navAt(Canvas c,Paint p,int[][] rectangles,
            int index,String label,boolean enabled,int hovered,float scale) {
        int[] r=rectangles[index];
        p.setStyle(Paint.Style.FILL);
        p.setColor(!enabled?Color.rgb(25,34,44)
            :index==hovered?Color.rgb(39,86,106):Color.rgb(23,48,68));
        c.drawRoundRect(r[0],r[1],r[2],r[3],7,7,p);
        p.setColor(!enabled?Color.rgb(65,84,95)
            :index==hovered?Color.rgb(251,214,103):Color.rgb(83,155,197));
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(index==hovered?3f:2f);
        c.drawRoundRect(r[0],r[1],r[2],r[3],7,7,p);
        p.setStyle(Paint.Style.FILL);
        p.setColor(enabled?Color.rgb(220,234,241):Color.rgb(98,112,122));
        fit(c,p,label,(r[0]+r[2])/2f,r[1]+33,r[2]-r[0]-14,
            17f*Math.min(scale,1.1f),true);
    }
    private static void fit(Canvas c,Paint p,String text,float x,float y,
                            float width,float requested,boolean center) {
        float size=requested;
        p.setTextAlign(center?Paint.Align.CENTER:Paint.Align.LEFT);
        p.setFakeBoldText(true);
        p.setTextSize(size);
        while(p.measureText(text)>width&&size>11f) {
            size-=.5f;p.setTextSize(size);
        }
        c.drawText(text,x,y,p);
        p.setTextAlign(Paint.Align.LEFT);
        p.setFakeBoldText(false);
    }
    /** Tiny original vector scenes: UI repaint only, shared by both stereo eyes. */
    private static void preview(Canvas c,Paint p,String id,
                                float x,float y,float r,float b,Bitmap atlas) {
        int[] cell=atlasCell(id);
        if(atlas!=null && !atlas.isRecycled() && cell!=null
            && atlas.getWidth()>=4*ATLAS_CELL_WIDTH
            && atlas.getHeight()>=2*ATLAS_CELL_HEIGHT) {
            p.setStyle(Paint.Style.FILL);
            p.setColor(Color.WHITE);
            p.setAlpha(255);
            p.setFilterBitmap(true);
            c.drawBitmap(atlas,
                new Rect(cell[0],cell[1],cell[0]+ATLAS_CELL_WIDTH,
                    cell[1]+ATLAS_CELL_HEIGHT),
                new Rect(Math.round(x),Math.round(y),Math.round(r),Math.round(b)),p);
            p.setFilterBitmap(false);
            return;
        }
        float w=r-x,h=b-y;
        p.setStyle(Paint.Style.FILL);
        int sky=Color.rgb(18,36,54),floor=Color.rgb(24,56,76);
        if(id.contains("red-ledger")){sky=Color.rgb(54,34,28);floor=Color.rgb(93,58,36);}
        else if(id.contains("gumball")){sky=Color.rgb(21,12,44);floor=Color.rgb(43,24,64);}
        else if(id.contains("omega")){sky=Color.rgb(39,43,32);floor=Color.rgb(85,88,53);}
        else if(id.contains("test-chamber")){sky=Color.rgb(7,51,55);floor=Color.rgb(18,104,109);}
        p.setColor(sky);c.drawRect(x,y,r,b,p);
        p.setColor(floor);c.drawRect(x,y+h*.55f,r,b,p);
        if(id.equals("breakwater-battery")) {
            p.setColor(Color.rgb(97,169,203));
            c.drawRect(x,y+h*.46f,r,y+h*.63f,p);
            p.setColor(Color.rgb(195,163,105));
            c.drawRect(x,y+h*.83f,r,b,p);
            p.setColor(Color.rgb(47,59,66));
            c.drawRect(x+w*.29f,b-h*.34f,x+w*.66f,b,p);
            c.drawRect(x+w*.43f,y+h*.32f,x+w*.51f,b-h*.19f,p);
        } else if(id.contains("red-ledger")) {
            p.setColor(Color.rgb(138,99,69));
            for(int k=0;k<3;k++)c.drawRect(x+9,y+24+k*36,r-9,y+30+k*36,p);
            for(int k=0;k<7;k++) {
                float bx=x+18+k*(w-34)/7f;
                p.setColor(k%2==0?Color.rgb(48,124,95):Color.rgb(172,85,61));
                c.drawRect(bx,y+44,bx+13,y+84,p);
                c.drawRect(bx+4,y+36,bx+9,y+44,p);
            }
            p.setColor(Color.rgb(28,20,17));c.drawRect(x,b-26,r,b,p);
        } else if(id.contains("gumball")) {
            for(int k=0;k<17;k++) {
                p.setColor(k%3==0?Color.rgb(246,109,184)
                    :k%3==1?Color.rgb(109,230,190):Color.rgb(242,192,70));
                c.drawCircle(x+12+(k*41)%(int)(w-24),
                    y+11+(k*31)%(int)(h-22),3+k%4,p);
            }
            p.setColor(Color.rgb(211,222,237));
            c.drawLine(x+w*.30f,b-15,x+w*.56f,y+25,p);
            c.drawLine(x+w*.56f,y+25,x+w*.74f,b-15,p);
            c.drawLine(x+w*.74f,b-15,x+w*.30f,b-15,p);
        } else if(id.contains("detachment")||id.contains("lantern")
                  ||id.contains("omega")) {
            p.setColor(Color.rgb(106,122,91));
            for(int k=1;k<7;k++)c.drawLine(x+k*w/7f,y,x+k*w/7f,b,p);
            for(int k=1;k<5;k++)c.drawLine(x,y+k*h/5f,r,y+k*h/5f,p);
            p.setColor(Color.rgb(238,188,86));
            c.drawLine(x+w*.12f,b-15,x+w*.8f,y+18,p);
            p.setColor(Color.rgb(91,208,175));
            c.drawCircle(x+w*.50f,y+h*.50f,13,p);
            p.setColor(Color.rgb(25,43,36));
            c.drawCircle(x+w*.50f,y+h*.50f,5,p);
        } else if(id.contains("test-chamber")) {
            p.setColor(Color.rgb(81,206,195));
            for(int k=0;k<11;k++)c.drawLine(x,y+k*h/10f,r,
                y+k*h/10f+(k%3-1)*9,p);
            p.setColor(Color.rgb(20,68,112));
            c.drawCircle(r-50,b-37,33,p);
            c.drawRect(r-62,b-37,r-28,b,p);
        } else {
            p.setColor(Color.rgb(146,163,175));
            for(int col=0;col<8;col++)for(int row=0;row<5;row++) {
                if((col+row)%2==0)
                    c.drawRect(x+25+col*25,y+18+row*23,
                        x+50+col*25,y+41+row*23,p);
            }
            p.setColor(Color.rgb(238,200,107));
            c.drawCircle(x+84,y+66,13,p);
            c.drawCircle(x+163,y+90,13,p);
        }
    }
    private NativeGamesGallery(){}
}
