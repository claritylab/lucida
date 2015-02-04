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

import org.testng.Assert;
import org.testng.annotations.Test;

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.State;
import edu.cmu.sphinx.fst.operations.Project;
import edu.cmu.sphinx.fst.operations.ProjectType;
import edu.cmu.sphinx.fst.semiring.TropicalSemiring;

/**
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class ProjectTest {
    /**
     * Create an fst to Project as per the example at
     * http://www.openfst.org/twiki/bin/view/FST/ProjectDoc
     * 
     * @return the created fst
     */
    private Fst createFst() {
        TropicalSemiring ts = new TropicalSemiring();
        Fst fst = new Fst(ts);

        State s1 = new State(ts.zero());
        State s2 = new State(ts.zero());
        State s3 = new State(ts.zero());
        State s4 = new State(2.f);

        // State 0
        fst.addState(s1);
        s1.addArc(new Arc(1, 5, 1.f, s2));
        s1.addArc(new Arc(2, 4, 3.f, s2));
        fst.setStart(s1);

        // State 1
        fst.addState(s2);
        s2.addArc(new Arc(3, 3, 7.f, s2));
        s2.addArc(new Arc(4, 2, 5.f, s3));

        // State 2
        fst.addState(s3);
        s3.addArc(new Arc(5, 1, 9.f, s4));

        // State 3
        fst.addState(s4);

        return fst;
    }

    /**
     * Create the Project on Input Labels as per the example at
     * http://www.openfst.org/twiki/bin/view/FST/ProjectDoc
     * 
     * @return the created fst
     */
    private Fst createPi() {
        TropicalSemiring ts = new TropicalSemiring();
        Fst fst = new Fst(ts);
        State s1 = new State(ts.zero());
        State s2 = new State(ts.zero());
        State s3 = new State(ts.zero());
        State s4 = new State(2.f);

        // State 0
        fst.addState(s1);
        s1.addArc(new Arc(1, 1, 1.f, s2));
        s1.addArc(new Arc(2, 2, 3.f, s2));
        fst.setStart(s1);

        // State 1
        fst.addState(s2);
        s2.addArc(new Arc(3, 3, 7.f, s2));
        s2.addArc(new Arc(4, 4, 5.f, s3));

        // State 2
        fst.addState(s3);
        s3.addArc(new Arc(5, 5, 9.f, s4));

        // State 3
        fst.addState(s4);

        return fst;
    }

    /**
     * Create the Project on Output Labels as per the example at
     * http://www.openfst.org/twiki/bin/view/FST/ProjectDoc
     * 
     * @return the created fst
     */
    private Fst createPo() {
        TropicalSemiring ts = new TropicalSemiring();
        Fst fst = new Fst(ts);

        State s1 = new State(ts.zero());
        State s2 = new State(ts.zero());
        State s3 = new State(ts.zero());
        State s4 = new State(2.f);

        // State 0
        fst.addState(s1);
        s1.addArc(new Arc(5, 5, 1.f, s2));
        s1.addArc(new Arc(4, 4, 3.f, s2));
        fst.setStart(s1);

        // State 1
        fst.addState(s2);
        s2.addArc(new Arc(3, 3, 7.f, s2));
        s2.addArc(new Arc(2, 2, 5.f, s3));

        // State 2
        fst.addState(s3);
        s3.addArc(new Arc(1, 1, 9.f, s4));

        // State 3
        fst.addState(s4);

        return fst;
    }

    @Test
    public void testProject() {
        System.out.println("Testing Project...");
        // Project on Input label
        Fst fst = createFst();
        Fst p = createPi();
        Project.apply(fst, ProjectType.INPUT);
        Assert.assertTrue(fst.equals(p));

        // Project on Output label
        fst = createFst();
        p = createPo();
        Project.apply(fst, ProjectType.OUTPUT);
        Assert.assertTrue(fst.equals(p));

        System.out.println("Testing Project Completed!\n");

    }

}
