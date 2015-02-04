/*
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2002-2004 Sun Microsystems, Inc.  
 * Portions Copyright 2002-2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.frontend.util;

import javax.swing.*;
import java.awt.*;

/**
 * @author Peter Wolf
 */
@SuppressWarnings("serial")
public class VUMeterPanel extends JPanel {

    public void setVu(VUMeter vu) {
        this.vu = vu;
    }


    VUMeter vu;
    boolean quit;
    Thread thread;


    public void start() {
        quit = false;
        thread = new VUMeterPanelThread();
        thread.start();
    }


    public void stop() {
        quit = true;
        boolean hasQuit = false;
        while (!hasQuit) {
            try {
                thread.join();
                hasQuit = true;
            } catch (InterruptedException e) {
            }
        }
    }


    class VUMeterPanelThread extends Thread {

        @Override
        public void run() {
            while (!quit) {
                repaint();  // probably this one should be replaced by a more appropriate method call in order to get rid of the annoying flickering
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {

                }
            }
        }
    }

    /**
     * Paint the component.  This will be called by AWT/Swing.
     *
     * @param g The <code>Graphics</code> to draw on.
     */
    @Override
    public void paintComponent(Graphics g) {
        super.paintComponent(g);

        if (vu != null) {
            paintVUMeter(g);
        }
    }


    final int numberOfLights = 50;
    final int greenLevel = (int) (numberOfLights * 0.3);
    final int yellowLevel = (int) (numberOfLights * 0.7);
    final int redLevel = (int) (numberOfLights * 0.9);


    public VUMeter getVu() {
        return vu;
    }


    private void paintVUMeter(Graphics g) {
        int level = (int) ((vu.getRmsDB() / vu.getMaxDB()) * numberOfLights);
        int peak = (int) ((vu.getPeakDB() / vu.getMaxDB()) * numberOfLights);

        assert level >= 0;
        assert level < numberOfLights;

        Dimension sz = getSize();
        int w = sz.width;
        int h = (sz.height / numberOfLights);

        g.setColor(Color.BLACK);
        g.fillRect(0, 0, sz.width - 1, sz.height - 1);

        for (int i = 0; i < level; i++) {
            setLevelColor(i, g);
            g.fillRect(1, sz.height - (i * h) + 1, w - 2, h - 2);
        }

        setLevelColor(peak, g);
        g.fillRect(1, sz.height - (peak * h) + 1, w - 2, h - 2);

    }


    private void setLevelColor(int i, Graphics g) {
        if (i < greenLevel)
            g.setColor(Color.BLUE);
        else if (i < yellowLevel)
            g.setColor(Color.GREEN);
        else if (i < redLevel)
            g.setColor(Color.YELLOW);
        else
            g.setColor(Color.RED);
    }
}

