package edu.cmu.sphinx.frontend.transform;

import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.PropertyException;

/** Applies the optimized MelCosine filter used in pocketsphinx to the given melspectrum. */
public class DiscreteCosineTransform2 extends DiscreteCosineTransform {

    public DiscreteCosineTransform2( int numberMelFilters, int cepstrumSize ) {
        super(numberMelFilters,cepstrumSize);
    }

    public DiscreteCosineTransform2( ) {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
    }


    /**
     * Apply the optimized MelCosine filter used in pocketsphinx to the given melspectrum.
     *
     * @param melspectrum the MelSpectrum data
     * @return MelCepstrum data produced by apply the MelCosine filter to the MelSpectrum data
     */
    @Override
    protected double[] applyMelCosine(double[] melspectrum) {

        // create the cepstrum
        double[] cepstrum = new double[cepstrumSize];
        double sqrt_inv_n = Math.sqrt(1.0 / numberMelFilters);
        double sqrt_inv_2n = Math.sqrt(2.0 / numberMelFilters);

        cepstrum[0] = melspectrum[0];
        for (int j = 1; j < numberMelFilters; j++) {
            cepstrum[0] += melspectrum[j];
        }

        cepstrum[0] *= sqrt_inv_n;

        if (numberMelFilters <= 0) {
            return cepstrum;
        }

        for (int i = 1; i < cepstrum.length; i++) {
            double[] melcosine_i = melcosine[i];
            int j = 0;
            cepstrum[i] = 0;
            for (j = 0; j < numberMelFilters; j++) {
                cepstrum[i] += (melspectrum[j] * melcosine_i[j]);
            }
            cepstrum[i] *= sqrt_inv_2n;
        }
        return cepstrum;
    }
}
