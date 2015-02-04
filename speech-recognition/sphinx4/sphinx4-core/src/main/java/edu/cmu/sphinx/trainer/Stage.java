/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.trainer;


/**
 * Indicates stages during training. The numbers prepended to the names indicate the order in which they are performed.
 * Consecutive numbers were avoided to allow for future expansion of alternative stages. The value of the numbers has no
 * bearing on the importance or duration of each stage, it is just an ordering method.
 */
public class Stage {

    private String name;

    /** Initialization stage. */
    public static final Stage _00_INITIALIZATION =
            new Stage("_00_INITIALIZATION");

    /** Context independent model training stage. */
    public static final Stage _10_CI_TRAIN = new Stage("_10_CI_TRAIN");

    /** Untied context dependent model training stage. */
    public static final Stage _20_UNTIED_CD_TRAIN =
            new Stage("_20_UNTIED_CD_TRAIN");

    /** State pruning stage. */
    public static final Stage _30_STATE_PRUNING =
            new Stage("_30_STATE_PRUNING");

    /** Tied context dependent model training stage. */
    public static final Stage _40_TIED_CD_TRAIN =
            new Stage("_40_TIED_CD_TRAIN");

    /** Copy models, possibly changing format. */
    public static final Stage _90_CP_MODEL =
            new Stage("_90_CP_MODEL");


    /** Constructs a Stage with the given name. */
    protected Stage(String name) {
        this.name = name;
    }


    /**
     * Returns true if the given Stage is equal to this Stage.
     *
     * @param stage the Stage to compare
     * @return true if they are the same, false otherwise
     */
    public boolean equals(Stage stage) {
        if (stage != null) {
            return toString().equals(stage.toString());
        } else {
            return false;
        }
    }


    /**
     * Returns true if the given String is equal to this Stage's name.
     *
     * @param stage the Stage to compare
     * @return true if they are the same, false otherwise
     */
    public boolean equals(String stage) {
        if (stage != null) {
            return toString().equals(stage);
        } else {
            return false;
        }
    }


    /**
     * Returns the name of this Stage.
     *
     * @return the name of this Stage.
     */
    @Override
    public String toString() {
        return name;
    }

}
