package dev.mrcalzon.water;
import javax.swing.*;
import java.awt.*;
import java.awt.geom.*;

/** Self-contained placement icon; no image file or game asset dependency. */
public final class WaterIcon implements Icon {
 public int getIconWidth(){return 24;}public int getIconHeight(){return 24;}
 public void paintIcon(Component component,Graphics graphics,int x,int y){var g=(Graphics2D)graphics.create();try{g.translate(x,y);g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);g.setColor(new Color(30,82,105));g.fillRoundRect(0,0,24,24,5,5);g.setColor(new Color(135,221,231));g.setStroke(new BasicStroke(1.8f));for(int row=0;row<3;row++){var p=new Path2D.Double();p.moveTo(3,7+row*5);p.curveTo(6,3+row*5,9,11+row*5,12,7+row*5);p.curveTo(15,3+row*5,18,11+row*5,21,7+row*5);g.draw(p);}}finally{g.dispose();}}
}
