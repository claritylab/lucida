package edu.cmu.sphinx.util.machlearn;

import java.io.Serializable;
import java.util.Arrays;

/** An real-valued observation. */
@SuppressWarnings("serial")
public class OVector implements Cloneable, Serializable {

    protected final double[] values;


    /** Constructs a new observation for a given feature-vector. */
    public OVector(double[] values) {
        this.values = values;
    }


    /** Creates a one-dimensional instance of this class. */
    public OVector(double value) {
        this(new double[]{value});
    }


    /**
     * Returns the values of this observation.
     *
     * @return the values
     */
    public double[] getValues() {
        return values;
    }


    /** Returns the dimension of this observation. */
    public int dimension() {
        return getValues().length;
    }


    @Override
    public boolean equals(Object obj) {
        return obj instanceof OVector && Arrays.equals(values, ((OVector) obj).values);

    }


    @Override
    public int hashCode() {
        return Arrays.hashCode(values);
    }


    @Override
    public String toString() {
        return Arrays.toString(values);
    }
}
