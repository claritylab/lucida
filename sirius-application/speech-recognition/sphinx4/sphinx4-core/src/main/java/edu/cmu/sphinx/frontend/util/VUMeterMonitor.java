package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.*;

import javax.swing.*;
import java.awt.*;

/**
 * A VU meter to be plugged into a front-end. Preferably this component should be plugged directly behind the
 * <code>DataBlocker</code> in order to ensure that only equally sized blocks of meaningful length are used for RMS
 * computation.
 * <p/>
 * Because vu-monitoring makes sense only for online speech processing the vu-meter will be visible only if data source
 * which precedes it is a <code>Microphone</code>.
 *
 * @author Holger Brandl
 */

public class VUMeterMonitor extends BaseDataProcessor {

    final VUMeter vumeter;
    final VUMeterPanel vuMeterPanel;
    final JDialog vuMeterDialog;


    public VUMeterMonitor() {
        vumeter = new VUMeter();

        vuMeterPanel = new VUMeterPanel();
        vuMeterPanel.setVu(vumeter);
        vuMeterPanel.start();

        vuMeterDialog = new JDialog();
        vuMeterDialog.setBounds(100, 100, 100, 400);

        vuMeterDialog.getContentPane().setLayout(new BorderLayout());
        vuMeterDialog.getContentPane().add(vuMeterPanel);

        vuMeterDialog.setVisible(true);
    }


    @Override
    public Data getData() throws DataProcessingException {
        Data d = getPredecessor().getData();

        // show the panel only if  a microphone is used as data source
        if (d instanceof DataStartSignal)
            vuMeterPanel.setVisible(FrontEndUtils.getFrontEndProcessor(this, Microphone.class) != null);

        if (d instanceof DoubleData)
            vumeter.calculateVULevels(d);

        return d;
    }


    public JDialog getVuMeterDialog() {
        return vuMeterDialog;
    }


    /** A little test-function which plugs a microphone directly into the vu-meter.
     * @param args
     * @throws edu.cmu.sphinx.frontend.DataProcessingException*/
    public static void main(String[] args) throws DataProcessingException {
        Microphone mic = new Microphone( 16000, 16, 1,
                          true, true, true, 10, false,
                          "selectChannel", 2, "default", 6400);

        mic.initialize();
        mic.startRecording();

        VUMeterMonitor monitor = new VUMeterMonitor();
        monitor.getVuMeterDialog().setModal(true);
        monitor.setPredecessor(mic);

        while (true) {
            monitor.getData();
        }
    }
}
