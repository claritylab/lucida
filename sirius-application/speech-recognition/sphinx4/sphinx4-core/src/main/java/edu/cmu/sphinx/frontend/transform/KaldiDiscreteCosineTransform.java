/*
 * Copyright 1999-2014 Carnegie Mellon University.  
 * Portions Copyright 2002-2004 Sun Microsystems, Inc.  
 * Portions Copyright 2002-2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */

package edu.cmu.sphinx.frontend.transform;

import java.util.Arrays;


/**
 * DCT implementation that conforms to one used in Kaldi.
 */
public class KaldiDiscreteCosineTransform extends DiscreteCosineTransform {

    public KaldiDiscreteCosineTransform(int numberMelFilters, int cepstrumSize)
    {
        super(numberMelFilters, cepstrumSize);
    }

    public KaldiDiscreteCosineTransform() {
    }

    @Override
    protected void computeMelCosine() {
        melcosine = new double[cepstrumSize][numberMelFilters];
        Arrays.fill(melcosine[0], Math.sqrt(1. / numberMelFilters));

        double normScale = Math.sqrt(2. / numberMelFilters);

        for (int i = 1; i < cepstrumSize; i++) {
            double frequency = Math.PI * i / numberMelFilters;

            for (int j = 0; j < numberMelFilters; j++)
                melcosine[i][j] = normScale * Math.cos(frequency * (j + 0.5));
        }
    }

    @Override
    protected double[] applyMelCosine(double[] melspectrum) {
        double[] cepstrum = new double[cepstrumSize];

        for (int i = 0; i < cepstrum.length; i++) {
                for (int j = 0; j < numberMelFilters; j++)
                    cepstrum[i] += melspectrum[j] * melcosine[i][j];
        }
        
        return cepstrum;
    }
}
