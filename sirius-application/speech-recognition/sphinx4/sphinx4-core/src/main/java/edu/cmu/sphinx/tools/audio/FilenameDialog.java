/*
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2002-2004 Sun Microsystems, Inc.  
 * Portions Copyright 2002-2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.tools.audio;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

/** Creates a dialog that prompts for a filename. */
@SuppressWarnings("serial")
public class FilenameDialog extends JDialog {

    String action;
    JTextField filename;


    /**
     * Class constructor.
     *
     * @param parent the parent of this dialog
     * @param modal  if true, this dialog box is modal
     * @param title  the title for the login box
     */
    public FilenameDialog(Frame parent,
                          boolean modal,
                          String title) {
        super(parent, modal);
        setTitle(title);
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent event) {
                setVisible(false);
            }
        });
        createFilenamePanel();
        pack();
    }


    /** Creates the filename panel. */
    void createFilenamePanel() {
        Container contentPane = getContentPane();
        GridBagLayout gridBag = new GridBagLayout();
        GridBagConstraints constraints;
        Insets insets;

        contentPane.setLayout(gridBag);
        filename = new JTextField(12);
        JLabel filenameLabel = new JLabel("Filename:");
        filenameLabel.setLabelFor(filename);

        insets = new Insets(12, 12, 0, 0);  // top, left, bottom, right
        constraints = new GridBagConstraints(
                0, 0, 1, 1,                     // x, y, width, height
                0.0, 0.0,                       // weightx, weighty
                GridBagConstraints.WEST,        // anchor
                GridBagConstraints.NONE,        // fill
                insets,                         // insets
                0, 0);                          // ipadx, ipady
        gridBag.setConstraints(filenameLabel, constraints);
        contentPane.add(filenameLabel);

        insets = new Insets(12, 7, 0, 12);  // top, left, bottom, right
        constraints = new GridBagConstraints(
                1, 0, 1, 1,                     // x, y, width, height
                1.0, 1.0,                       // weightx, weighty
                GridBagConstraints.WEST,        // anchor
                GridBagConstraints.HORIZONTAL,  // fill
                insets,                         // insets
                0, 0);                          // ipadx, ipady
        gridBag.setConstraints(filename, constraints);
        contentPane.add(filename);

        /* BUTTON PANEL
         */
        JButton okButton = new JButton("Save");
        okButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent event) {
                setVisible(false);
            }
        });

        insets = new Insets(0, 12, 12, 12); // top, left, bottom, right
        constraints = new GridBagConstraints(
                0, 2, 2, 1,                     // x, y, width, height
                1.0, 1.0,                       // weightx, weighty
                GridBagConstraints.EAST,        // anchor
                GridBagConstraints.NONE,        // fill
                insets,                         // insets
                0, 0);                          // ipadx, ipady
        gridBag.setConstraints(okButton, constraints);
        contentPane.add(okButton);

        getRootPane().setDefaultButton(okButton);
    }


    /**
     * Gets the user ID.
     *
     * @return the user ID
     */
    public String getFilename() {
        return filename.getText();
    }


    /** Debug and example use. */
    public static void main(String args[]) {
        JFrame frame = new JFrame();
        frame.setTitle("Debug");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(false);

        FilenameDialog dialog = new FilenameDialog(frame, true, "Save as...");

        System.out.println("Showing dialog...");
        dialog.setVisible(true);

        String filename = dialog.getFilename();
        System.out.println("Filename: " + filename
                + " (length = " + filename.length() + ')');
        System.exit(0);
    }
}
