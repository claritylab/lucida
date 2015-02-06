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

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataEndSignal;
import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.frontend.FrontEnd;
import edu.cmu.sphinx.frontend.util.StreamDataSource;

import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.awt.image.FilteredImageSource;
import java.awt.image.ImageFilter;
import java.awt.image.ReplicateScaleFilter;
import java.util.ArrayList;

/** Converts a set of log magnitude Spectrum data into a graphical representation. */
@SuppressWarnings("serial")
public class SpectrogramPanel extends JPanel {

    /** Where the spectrogram will live. */
    protected BufferedImage spectrogram;

    /** A scaled version of the spectrogram image. */
    protected Image scaledSpectrogram;

    /** The zooming factor. */
    protected float zoom = 1.0f;

    /** Offset factor - what will be subtracted from the image to adjust for noise level. */
    protected double offsetFactor;

    /** The audio data. */
    protected AudioData audio;


    /** The frontEnd (the source of features */
    protected FrontEnd frontEnd;

    /** The source of audio (the first stage of the frontend) */
    protected StreamDataSource dataSource;


    /** Creates a new <code>JPanel</code> with a double buffer and a flow layout. */
    public SpectrogramPanel() {
    }


    /**
     * Creates a new SpectrogramPanel for the given AudioData.
     *
     * @param frontEnd   the front end to use
     * @param dataSource the source of audio
     * @param audioData  the AudioData
     */
    public SpectrogramPanel(FrontEnd frontEnd,
                            StreamDataSource dataSource, AudioData audioData) {
        audio = audioData;
        this.frontEnd = frontEnd;
        this.dataSource = dataSource;
        audio.addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent event) {
                computeSpectrogram();
            }
        });
    }


    /** Actually creates the Spectrogram image. */
    protected void computeSpectrogram() {
        try {
            AudioDataInputStream is = new AudioDataInputStream(audio);
            dataSource.setInputStream(is);

            /* Run through all the spectra one at a time and convert
             * them to an log intensity value.
             */
            ArrayList<double[]> intensitiesList = new ArrayList<double[]>();
            double maxIntensity = Double.MIN_VALUE;
            Data spectrum = frontEnd.getData();

            while (!(spectrum instanceof DataEndSignal)) {
                if (spectrum instanceof DoubleData) {
                    double[] spectrumData = ((DoubleData) spectrum).getValues();
                    double[] intensities = new double[spectrumData.length];
                    for (int i = 0; i < intensities.length; i++) {
                        /*
			 * A very small intensity is, for all intents
                         * and purposes, the same as 0.
                         */
                        intensities[i] = Math.max(Math.log(spectrumData[i]),
                                0.0);
                        if (intensities[i] > maxIntensity) {
                            maxIntensity = intensities[i];
                        }
                    }
                    intensitiesList.add(intensities);
                }
                spectrum = frontEnd.getData();
            }
            is.close();

            int width = intensitiesList.size();
            int height = (intensitiesList.get(0)).length;
            int maxYIndex = height - 1;
            Dimension d = new Dimension(width, height);

            setMinimumSize(d);
            setMaximumSize(d);
            setPreferredSize(d);

            /* Create the image for displaying the data.
             */
            spectrogram = new BufferedImage(width,
                    height,
                    BufferedImage.TYPE_INT_RGB);

            /* Set scaleFactor so that the maximum value, after removing
            * the offset, will be 0xff.
            */
            double scaleFactor = ((0xff + offsetFactor) / maxIntensity);

            for (int i = 0; i < width; i++) {
                double[] intensities = intensitiesList.get(i);
                for (int j = maxYIndex; j >= 0; j--) {

                    /* Adjust the grey value to make a value of 0 to mean
                    * white and a value of 0xff to mean black.
                    */
                    int grey = (int) (intensities[j] * scaleFactor
                            - offsetFactor);
                    grey = Math.max(grey, 0);
                    grey = 0xff - grey;

                    /* Turn the grey into a pixel value.
                    */
                    int pixel = ((grey << 16) & 0xff0000)
                            | ((grey << 8) & 0xff00)
                            | (grey & 0xff);

                    spectrogram.setRGB(i, maxYIndex - j, pixel);
                }
            }
            ImageFilter scaleFilter =
                    new ReplicateScaleFilter((int) (zoom * width), height);
            scaledSpectrogram =
                    createImage(new FilteredImageSource(spectrogram.getSource(),
                            scaleFilter));
            Dimension sz = getSize();
            repaint(0, 0, 0, sz.width - 1, sz.height - 1);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    /**
     * Updates the offset factor used to calculate the greyscale values from the intensities.  This also calculates and
     * populates all the greyscale values in the image.
     *
     * @param offsetFactor the offset factor used to calculate the greyscale values from the intensities; this is used
     *                     to adjust the level of background noise that shows up in the image
     */
    public void setOffsetFactor(double offsetFactor) {
        this.offsetFactor = offsetFactor;
        computeSpectrogram();
    }


    /** Zoom the image, preparing for new display. */
    protected void zoomSet(float zoom) {
        this.zoom = zoom;
        if (spectrogram != null) {
            int width = spectrogram.getWidth();
            int height = spectrogram.getHeight();

            ImageFilter scaleFilter =
                    new ReplicateScaleFilter((int) (zoom * width), height);
            scaledSpectrogram =
                    createImage(new FilteredImageSource(spectrogram.getSource(),
                            scaleFilter));
            Dimension d = new Dimension((int) (width * zoom), height);
            setMinimumSize(d);
            setMaximumSize(d);
            setPreferredSize(d);
            repaint();
        }
    }


    /**
     * Paint the component.  This will be called by AWT/Swing.
     *
     * @param g The <code>Graphics</code> to draw on.
     */
    @Override
    public void paint(Graphics g) {
        /**
         * Fill in the whole image with white.
         */
        Dimension sz = getSize();

        g.setColor(Color.WHITE);
        g.fillRect(0, 0, sz.width - 1, sz.height - 1);

        if (spectrogram != null) {

            g.drawImage(scaledSpectrogram, 0, 0, null);
        }
    }
}
