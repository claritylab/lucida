/*
 * Copyright 1999-2004 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.result;

import edu.cmu.sphinx.decoder.search.AlternateHypothesisManager;
import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.linguist.HMMSearchState;
import edu.cmu.sphinx.linguist.SearchState;
import edu.cmu.sphinx.linguist.UnitSearchState;
import edu.cmu.sphinx.linguist.WordSearchState;

import java.io.FileWriter;
import java.io.IOException;
import java.util.*;

/**
 * Dumps out the GDL graph of all the result token chains in a Result, as well as all the alternate hypotheses along
 * those chains.
 */
public class TokenGraphDumper {

    private final AlternateHypothesisManager loserManager;
    private final Result result;
    private final Map<Token, Integer> tokenIDMap;
    private final Set<Token> dumpedTokens;
    private int ID;


    /**
     * Constructs a TokenGraphDumper from the given result.
     *
     * @param result The result which search space we want to dump.
     */
    public TokenGraphDumper(Result result) {
        this.result = result;
        this.loserManager = result.getAlternateHypothesisManager();
        tokenIDMap = new HashMap<Token, Integer>();
        dumpedTokens = new HashSet<Token>();
    }


    /**
     * Dumps the GDL output of the search space to the given file.
     *
     * @param title    the title of the GDL graph
     * @param fileName
     */
    public void dumpGDL(String title, String fileName) {
        try {
            System.err.println("Dumping " + title + " to " + fileName);
            FileWriter f = new FileWriter(fileName);
            f.write(dumpGDL(title));
            f.close();
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
    }


    /**
     * Dumps the GDL output.
     *
     * @param title the title of the GDL graph
     * @return the GDL output string
     */
    public String dumpGDL(String title) {
        StringBuilder gdl = new StringBuilder("graph: {\n");
        gdl.append("title: \"").append(title).append("\"\n");
        gdl.append("display_edge_labels: yes\n");

        for (Token token : result.getResultTokens()) {
            gdl.append(dumpTokenGDL(token));
        }

        gdl.append("}\n");
        return gdl.toString();
    }


    /**
     * Dumps the GDL output for a token, and any of its predecessors or alternate hypotheses.
     *
     * @param token the token to dump
     * @return the GDL output string
     */
    private String dumpTokenGDL(Token token) {

        if (dumpedTokens.contains(token)) {
            return "";
        } else {
            String label = ("[" + token.getAcousticScore() + token.getInsertionScore() + ',' +
                    token.getLanguageScore() + ']');
            if (token.isWord()) {
                label = token.getWord().getSpelling() + label;
            }

            String color = null;

            if (token.getSearchState() != null) {
                color = getColor(token.getSearchState());
            }

            StringBuilder gdl = new StringBuilder().append("node: { title: \"").append(getTokenID(token))
                .append("\" label: \"").append(label).append("\" color: ");
            if (color != null) {
                gdl.append(color).append(" }");
            } else {
                gdl.append(" }");
            }
            gdl.append('\n');

            dumpedTokens.add(token);

            if (token.getPredecessor() != null) {
                gdl.append("edge: { sourcename: \"").append(getTokenID(token))
                    .append("\" targetname: \"").append(getTokenID(token.getPredecessor()))
                    .append("\" }").append('\n').append(dumpTokenGDL(token.getPredecessor()));
            }

            if (loserManager != null) {
                List<Token> list = loserManager.getAlternatePredecessors(token);
                if (list != null) {
                    for (Token loser : list) {
                        gdl.append("edge: { sourcename: \"").append(getTokenID(token))
                            .append("\" targetname: \"").append(getTokenID(loser))
                            .append("\" }").append('\n').append(dumpTokenGDL(loser));
                    }
                }
            }
            return gdl.toString();
        }
    }


    /**
     * Gets the color for a particular state
     *
     * @param state the state
     * @return its color
     */
    private String getColor(SearchState state) {
        String color = "lightred";
        if (state.isFinal()) {
            color = "magenta";
        } else if (state instanceof UnitSearchState) {
            color = "green";
        } else if (state instanceof WordSearchState) {
            color = "lightblue";
        } else if (state instanceof HMMSearchState) {
            color = "orange";
        }
        return color;
    }


    /**
     * Returns the next available token ID.
     *
     * @param token the token for which we want an ID
     * @return the next available token ID
     */
    private Integer getTokenID(Token token) {
        Integer id = tokenIDMap.get(token);
        if (id == null) {
            id = ID++;
            tokenIDMap.put(token, id);
        }
        return id;
    }
}
