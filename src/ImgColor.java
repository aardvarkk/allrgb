// Source: http://codegolf.stackexchange.com/a/23503/20505

import java.awt.Color;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.LinkedList;
import javax.imageio.ImageIO;

public class ImgColor {

    private static class Point {
        public int x, y;
        public color c;

        public Point(int x, int y, color c) {
            this.x = x;
            this.y = y;
            this.c = c;
        }
    }

    private static class color {
        char r, g, b;

        public color(int i, int j, int k) {
            r = (char) i;
            g = (char) j;
            b = (char) k;
        }
    }

    public static LinkedList<Point> listFromImg(String path) {
        LinkedList<Point> ret = new LinkedList<>();
        BufferedImage bi = null;
        try {
            bi = ImageIO.read(new File(path));
        } catch (IOException e) {
            e.printStackTrace();
        }
        for (int x = 0; x < 4096; x++) {
            for (int y = 0; y < 4096; y++) {
                Color c = new Color(bi.getRGB(x, y));
                ret.add(new Point(x, y, new color(c.getRed(), c.getGreen(), c.getBlue())));
            }
        }
        Collections.shuffle(ret);
        return ret;
    }

    public static LinkedList<color> allColors() {
        LinkedList<color> colors = new LinkedList<>();
        for (int r = 0; r < 256; r++) {
            for (int g = 0; g < 256; g++) {
                for (int b = 0; b < 256; b++) {
                    colors.add(new color(r, g, b));
                }
            }
        }
        Collections.shuffle(colors);
        return colors;
    }

    public static Double cDelta(color a, color b) {
        return Math.pow(a.r - b.r, 2) + Math.pow(a.g - b.g, 2) + Math.pow(a.b - b.b, 2);
    }

    public static void main(String[] args) {
        BufferedImage img = new BufferedImage(4096, 4096, BufferedImage.TYPE_INT_RGB);
        LinkedList<Point> orig = listFromImg(args[0]);
        LinkedList<color> toDo = allColors();

        Point p = null;
        while (orig.size() > 0 && (p = orig.pop()) != null) {
            color chosen = toDo.pop();
            for (int i = 0; i < Math.min(100, toDo.size()); i++) {
                color c = toDo.pop();
                if (cDelta(c, p.c) < cDelta(chosen, p.c)) {
                    toDo.add(chosen);
                    chosen = c;
                } else {
                    toDo.add(c);
                }
            }
            img.setRGB(p.x, p.y, new Color(chosen.r, chosen.g, chosen.b).getRGB());
        }
        try {
            ImageIO.write(img, "PNG", new File(args[1]));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}