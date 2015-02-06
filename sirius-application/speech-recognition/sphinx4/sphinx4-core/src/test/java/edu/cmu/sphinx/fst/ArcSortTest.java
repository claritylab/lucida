/**
 * 
 * Copyright 1999-2012 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.fst;

import static edu.cmu.sphinx.fst.operations.ArcSort.apply;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.not;

import org.testng.annotations.Test;

import edu.cmu.sphinx.fst.operations.ILabelCompare;
import edu.cmu.sphinx.fst.operations.OLabelCompare;
import edu.cmu.sphinx.fst.semiring.TropicalSemiring;

public class ArcSortTest {

    /**
     * Create an output label sorted fst as per the example at
     * http://www.openfst.org/twiki/bin/view/FST/ArcSortDoc
     * 
     * @return the created fst
     */
    private Fst createOsorted() {
        Fst fst = new Fst(new TropicalSemiring());

        State s1 = new State(0.f);
        State s2 = new State(0.f);
        State s3 = new State(0.f);

        // State 0
        fst.addState(s1);
        s1.addArc(new Arc(4, 1, 0.f, s3));
        s1.addArc(new Arc(5, 2, 0.f, s3));
        s1.addArc(new Arc(2, 3, 0.f, s2));
        s1.addArc(new Arc(1, 4, 0.f, s2));
        s1.addArc(new Arc(3, 5, 0.f, s2));

        // State 1
        fst.addState(s2);
        s2.addArc(new Arc(3, 1, 0.f, s3));
        s2.addArc(new Arc(1, 2, 0.f, s3));
        s2.addArc(new Arc(2, 3, 0.f, s2));

        // State 2 (final)
        fst.addState(s3);

        return fst;
    }

    /**
     * Create an input label sorted fst as per the example at
     * http://www.openfst.org/twiki/bin/view/FST/ArcSortDoc
     * 
     * @return the created fst
     */
    private Fst createIsorted() {
        Fst fst = new Fst(new TropicalSemiring());

        State s1 = new State(0.f);
        State s2 = new State(0.f);
        State s3 = new State(0.f);

        // State 0
        fst.addState(s1);
        s1.addArc(new Arc(1, 4, 0.f, s2));
        s1.addArc(new Arc(2, 3, 0.f, s2));
        s1.addArc(new Arc(3, 5, 0.f, s2));
        s1.addArc(new Arc(4, 1, 0.f, s3));
        s1.addArc(new Arc(5, 2, 0.f, s3));

        // State 1
        fst.addState(s2);
        s2.addArc(new Arc(1, 2, 0.f, s3));
        s2.addArc(new Arc(2, 3, 0.f, s2));
        s2.addArc(new Arc(3, 1, 0.f, s3));

        // State 2 (final)
        fst.addState(s3);

        return fst;
    }

    /**
     * Create an unsorted fst as per the example at
     * http://www.openfst.org/twiki/bin/view/FST/ArcSortDoc
     * 
     * @return the created fst
     */
    private Fst createUnsorted() {
        Fst fst = new Fst(new TropicalSemiring());

        State s1 = new State(0.f);
        State s2 = new State(0.f);
        State s3 = new State(0.f);

        // State 0
        fst.addState(s1);
        s1.addArc(new Arc(1, 4, 0.f, s2));
        s1.addArc(new Arc(3, 5, 0.f, s2));
        s1.addArc(new Arc(2, 3, 0.f, s2));
        s1.addArc(new Arc(5, 2, 0.f, s3));
        s1.addArc(new Arc(4, 1, 0.f, s3));

        // State 1
        fst.addState(s2);
        s2.addArc(new Arc(2, 3, 0.f, s2));
        s2.addArc(new Arc(3, 1, 0.f, s3));
        s2.addArc(new Arc(1, 2, 0.f, s3));

        // State 2 (final)
        fst.addState(s3);

        return fst;
    }

    @Test
    public void testArcSort() {
        // Input label sort test
        Fst fst1 = createUnsorted();
        Fst fst2 = createIsorted();
        assertThat(fst1, not(equalTo(fst2)));
        apply(fst1, new ILabelCompare());
        assertThat(fst1, equalTo(fst2));

        // Output label sort test
        fst1 = createUnsorted();
        fst2 = createOsorted();
        assertThat(fst1, not(equalTo(fst2)));
        apply(fst1, new OLabelCompare());
        assertThat(fst1, equalTo(fst2));
    }

}
