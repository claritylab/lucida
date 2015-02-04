/*
 * Copyright 1999-2004 Carnegie Mellon University. Portions Copyright 2004 Sun
 * Microsystems, Inc. Portions Copyright 2004 Mitsubishi Electric Research
 * Laboratories. All Rights Reserved. Use is subject to license terms. See the
 * file "license.terms" for information on usage and redistribution of this
 * file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package edu.cmu.sphinx.speakerid;

import java.io.InputStream;
import java.util.ArrayList;


public interface Identification {

    ArrayList<SpeakerCluster> cluster(InputStream stream);

    ArrayList<SpeakerCluster> cluster(ArrayList<float[]> features);
}
