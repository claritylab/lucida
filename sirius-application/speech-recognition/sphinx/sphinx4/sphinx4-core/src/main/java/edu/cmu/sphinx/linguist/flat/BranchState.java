package edu.cmu.sphinx.linguist.flat;


/** Represents a branching node in a grammar */

@SuppressWarnings("serial")
public class BranchState extends SentenceHMMState {

    /**
     * Creates a branch state
     *
     * @param nodeID the grammar node id
     */
    public BranchState(String leftContext, String rightContext, int nodeID) {
        super("B[" + leftContext + "," +
                rightContext + "]", null, nodeID);
    }


    /**
     * Retrieves a short label describing the type of this state. Typically, subclasses of SentenceHMMState will
     * implement this method and return a short (5 chars or less) label
     *
     * @return the short label.
     */
    @Override
    public String getTypeLabel() {
        return "Brnch";
    }


    /**
     * Returns the state order for this state type
     *
     * @return the state order
     */
    @Override
    public int getOrder() {
        return 2;
    }
}
