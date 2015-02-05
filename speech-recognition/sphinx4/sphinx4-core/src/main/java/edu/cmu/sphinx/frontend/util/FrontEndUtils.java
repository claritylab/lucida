package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.DataProcessor;
import edu.cmu.sphinx.frontend.FrontEnd;

/**
 * Some little helper methods to ease the handling of frontend-processor chains.
 *
 * @author Holger Brandl
 */
public class FrontEndUtils {


    /** Returns a the next <code>DataProcessor</code> of type <code>predecClass</code> which precedes <code>dp</code>
     * @param dp
     * @param predecClass
     */
    public static <T extends DataProcessor> T getFrontEndProcessor(DataProcessor dp, Class<T> predecClass) {
        while (!predecClass.isInstance(dp)) {
            if (dp instanceof FrontEnd)
                dp = ((FrontEnd) dp).getLastDataProcessor();
            else
                dp = dp.getPredecessor();

            if (dp == null)
                return null;
        }


        return predecClass.cast(dp);
    }
}
