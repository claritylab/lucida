/**
 * 
 */
package edu.cmu.sphinx.fst;

import java.util.Arrays;
import java.util.Comparator;

/**
 * The fst's immutable state implementation.
 * 
 * holds its outgoing {@link edu.cmu.sphinx.fst.Arc} objects in a fixed size
 * array not allowing additions/deletions.
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class ImmutableState extends State {

    // Outgoing arcs
    private Arc[] arcs = null;

    /**
     * Default protected constructor.
     * 
     * An ImmutableState cannot be created directly. It needs to be deserialized
     * as part of an ImmutableFst.
     * 
     * @see edu.cmu.sphinx.fst.ImmutableFst#loadModel(String)
     * 
     */
    protected ImmutableState() {
    }

    /**
     * Constructor specifying the capacity of the arcs array.
     * 
     * @param numArcs
     */
    protected ImmutableState(int numArcs) {
        super(0);
        this.initialNumArcs = numArcs;
        arcs = new Arc[numArcs];
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.State#arcSort(java.util.Comparator)
     */
    @Override
    public void arcSort(Comparator<Arc> cmp) {
        Arrays.sort(arcs, cmp);
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.State#addArc(edu.cmu.sphinx.fst.Arc)
     */
    @Override
    public void addArc(Arc arc) {
        throw new IllegalArgumentException(
                "You cannot modify an ImmutableState.");
    }

    /**
     * Set an arc at the specified position in the arcs' array.
     * 
     * @param index the position to the arcs' array
     * @param arc the arc value to set
     */
    @Override
    public void setArc(int index, Arc arc) {
        arcs[index] = arc;
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.State#deleteArc(int)
     */
    @Override
    public Arc deleteArc(int index) {
        throw new IllegalArgumentException(
                "You cannot modify an ImmutableState.");
    }

    /**
     * Set the state's arcs array
     * 
     * @param arcs the arcs array to set
     */
    public void setArcs(Arc[] arcs) {
        this.arcs = arcs;
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.State#getNumArcs()
     */
    @Override
    public int getNumArcs() {
        return initialNumArcs;
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.State#getArc(int)
     */
    @Override
    public Arc getArc(int index) {
        return this.arcs[index];
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#hashCode()
     */
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + id;
        return result;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (getClass() != obj.getClass())
            return false;
        ImmutableState other = (ImmutableState) obj;
        if (!Arrays.equals(arcs, other.arcs))
            return false;
        if (!super.equals(obj))
            return false;
        return true;
    }
}
