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

package edu.cmu.sphinx.fst.operations;

/**
 * Enum used in {@link edu.cmu.sphinx.fst.operations.Project} operation.
 * 
 * It specifies whether the Project operation will take place on input or output
 * labels
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public enum ProjectType {
    INPUT, OUTPUT
}
