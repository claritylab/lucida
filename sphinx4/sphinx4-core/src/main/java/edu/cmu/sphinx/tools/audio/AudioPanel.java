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

package edu.cmu.sphinx.tools.audio;

import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.*;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.util.Arrays;

/** Provides an interface to view and play back various forms of an audio signal. */
@SuppressWarnings("serial")
public class AudioPanel extends JPanel
        implements MouseMotionListener, MouseListener {

    private final AudioData audio;
    private float[] labelTimes;
    private String[] labels;
    private float xScale;
    private final float yScale;
    private final float originalXScale;
    private int xDragStart;
    private int xDragEnd;
    protected int selectionStart = -1;
    protected int selectionEnd = -1;


    /**
     * Creates a new AudioPanel.  The scale factors represent how much to scale the audio.  A scaleX factor of 1.0f
     * means one pixel per sample, and a scaleY factor of 1.0f means one pixel per resolution of the sample (e.g., a
     * scale of 1.0f would take 2**16 pixels).
     *
     * @param audioData the AudioData to draw
     * @param scaleX    how much to scale the width of the audio
     * @param scaleY    how much to scale the height
     */
    public AudioPanel(AudioData audioData,
                      float scaleX,
                      float scaleY) {
        this.audio = audioData;
        labelTimes = new float[0];
        labels = new String[0];
        this.xScale = scaleX;
        this.yScale = scaleY;
        this.originalXScale = this.xScale;

        int width = (int) (audio.getAudioData().length * xScale);
        int height = (int) ((1 << 16) * yScale);

        setPreferredSize(new Dimension(width, height));
        setBackground(Color.white);

        audio.addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent event) {
                int width = (int) (audio.getAudioData().length * xScale);
                int height = (int) ((1 << 16) * yScale);

                labelTimes = new float[0];
                labels = new String[0];

                setSelectionStart(-1);
                setSelectionEnd(-1);

                setPreferredSize(new Dimension(width, height));
                Dimension sz = getSize();

                revalidate();
                repaint(0, 0, 0, sz.width, sz.height);
            }
        });

        addMouseMotionListener(this);
        addMouseListener(this);
        setFocusable(true);
        requestFocus();
    }


    /** Sets the labels to be used when drawing this panel. */
    public void setLabels(float[] labelTimes, String[] labels) {
        this.labelTimes = labelTimes;
        this.labels = labels;
        repaint();
    }


    /** Sets the zoom, adjusting the scroll bar in the process. */
    protected void zoomSet(float zoom) {
        xScale = originalXScale * zoom;
        int width = (int) (audio.getAudioData().length * xScale);
        int height = (int) ((1 << 16) * yScale);

        setPreferredSize(new Dimension(width, height));
        revalidate();
        repaint();
    }


    /**
     * Repaints the component with the given Graphics.
     *
     * @param g the Graphics to use to repaint the component.
     */
    @Override
    public void paintComponent(Graphics g) {
        int pos, index;
        int length;

        super.paintComponent(g);

        Dimension sz = getSize();
        int gZero = sz.height / 2;
        short[] audioData = audio.getAudioData();

        /**
         * Only draw what is in the viewport.
         */
        JViewport viewport = getViewport();
        if (viewport != null) {
            Rectangle r = viewport.getViewRect();
            pos = (int) r.getX();
            length = (int) r.getWidth();
        } else {
            pos = 0;
            length = (int) (audioData.length * xScale);
        }

        /**
         * Fill in the whole image with white.
         */
        g.setColor(Color.WHITE);
        g.fillRect(pos, 0, length, sz.height - 1);

        /**
         * Now fill in the audio selection area as gray.
         */
        index = Math.max(0, getSelectionStart());
        int start = (int) (index * xScale);
        index = getSelectionEnd();
        if (index == -1) {
            index = audioData.length - 1;
        }
        int end = (int) (index * xScale);
        g.setColor(Color.LIGHT_GRAY);
        g.fillRect(start, 0,
                end - start, sz.height - 1);

        /* Now scale the audio data and draw it.
         */
        int[] x = new int[length];
        int[] y = new int[length];
        for (int i = 0; i < length; i++) {
            x[i] = pos;
            index = (int) (pos / xScale);
            if (index < audioData.length) {
                y[i] = gZero - (int) (audioData[index] * yScale);
            } else {
                break;
            }
            pos++;
        }
        g.setColor(Color.RED);
        g.drawPolyline(x, y, length);

        /**
         * Now draw the labels.
         */
        for (int i = 0; i < labelTimes.length; i++) {
            pos = (int) (xScale
                    * labelTimes[i]
                    * audio.getAudioFormat().getSampleRate());
            g.drawLine(pos, 0, pos, sz.height - 1);
            g.drawString(labels[i], pos + 5, sz.height - 5);
        }
    }


    /** Finds the JViewport enclosing this component. */
    private JViewport getViewport() {
        Container p = getParent();
        if (p instanceof JViewport) {
            Container gp = p.getParent();
            if (gp instanceof JScrollPane) {
                JScrollPane scroller = (JScrollPane) gp;
                JViewport viewport = scroller.getViewport();
                if (viewport == null || viewport.getView() != this) {
                    return null;
                } else {
                    return viewport;
                }
            }
        }
        return null;
    }


    /**
     * Returns the index of the sample representing the start of the selection.  -1 means the very beginning.
     *
     * @return the start of the selection
     * @see #crop
     * @see #getSelectionEnd
     */
    public int getSelectionStart() {
        return selectionStart;
    }


    /**
     * Sets the index of the sample of representing the start of the selection.  -1 means the very beginning.
     *
     * @param newStart the new selection start
     * @see #crop
     * @see #setSelectionEnd
     */
    public void setSelectionStart(int newStart) {
        selectionStart = newStart;
        if (selectionEnd != -1) {
            if (selectionEnd < selectionStart) {
                selectionEnd = selectionStart;
            }
        }
    }


    /**
     * Returns the index of the sample representing the end of the selection.  -1 means the very end.
     *
     * @return the end of the selection
     * @see #crop
     * @see #getSelectionStart
     */
    public int getSelectionEnd() {
        return selectionEnd;
    }


    /**
     * Sets the index of the sample of representing the end of the selection.  -1 means the very end.
     *
     * @param newEnd the new selection end
     * @see #crop
     * @see #setSelectionStart
     */
    public void setSelectionEnd(int newEnd) {
        selectionEnd = newEnd;
        if (selectionEnd != -1) {
            if (selectionStart > selectionEnd) {
                selectionStart = selectionEnd;
            }
        }
    }


    /**
     * Crops the audio data between the start and end selections. All audio data outside the region will be permanently
     * lost. The selection will be reset to the very beginning and very end of the cropped clip.
     *
     * @see #getSelectionStart
     * @see #getSelectionEnd
     */
    public void crop() {
        short[] shorts = audio.getAudioData();
        int start = Math.max(0, getSelectionStart());
        int end = getSelectionEnd();
        if (end == -1) {
            end = shorts.length;
        }
        audio.setAudioData(Arrays.copyOfRange(shorts, start, end));

        setSelectionStart(-1);
        setSelectionEnd(-1);
    }


    /** Clears the current selection. */
    public void selectAll() {
        setSelectionStart(-1);
        setSelectionEnd(-1);
        repaint();
    }


    /**
     * When the mouse is pressed, we update the selection in the audio.
     *
     * @param evt the mouse pressed event
     */
    public void mousePressed(MouseEvent evt) {
        xDragStart = Math.max(0, evt.getX());
        setSelectionStart((int) (xDragStart / xScale));
        setSelectionEnd((int) (xDragStart / xScale));
        repaint();
    }


    /**
     * When the mouse is dragged, we update the selection in the audio.
     *
     * @param evt the mouse dragged event
     */
    public void mouseDragged(MouseEvent evt) {
        xDragEnd = evt.getX();
        if (xDragEnd < (int) (getSelectionStart() * xScale)) {
            setSelectionStart((int) (xDragEnd / xScale));
        } else {
            setSelectionEnd((int) (xDragEnd / xScale));
        }
        repaint();
    }


    public void mouseReleased(MouseEvent evt) {
    }


    public void mouseMoved(MouseEvent evt) {
    }


    public void mouseEntered(MouseEvent evt) {
    }


    public void mouseExited(MouseEvent evt) {
    }


    public void mouseClicked(MouseEvent evt) {
    }
}
