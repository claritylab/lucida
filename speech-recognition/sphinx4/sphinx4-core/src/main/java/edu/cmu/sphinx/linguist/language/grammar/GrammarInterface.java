package edu.cmu.sphinx.linguist.language.grammar;

import java.util.Set;

/**
 * Copyright 1999-2006 Carnegie Mellon University. Portions Copyright 2002 Sun Microsystems, Inc. Portions Copyright
 * 2002 Mitsubishi Electric Research Laboratories. All Rights Reserved.  Use is subject to license terms.
 * <p/>
 * See the file "license.terms" for information on usage and redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 * <p/>
 * User: Peter Wolf Date: Jan 10, 2006 Time: 9:56:09 AM
 */
public interface GrammarInterface {

    GrammarNode getInitialNode();


    Set<GrammarNode> getGrammarNodes();
}
