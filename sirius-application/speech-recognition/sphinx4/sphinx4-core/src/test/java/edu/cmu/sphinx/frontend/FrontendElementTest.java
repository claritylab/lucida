package edu.cmu.sphinx.frontend;

import static java.lang.Double.parseDouble;
import static java.lang.Float.parseFloat;
import static java.lang.Integer.parseInt;
import static java.lang.Math.abs;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.closeTo;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.instanceOf;

import java.io.*;
import java.net.URL;

import org.testng.Assert;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.endpoint.SpeechEndSignal;
import edu.cmu.sphinx.frontend.endpoint.SpeechStartSignal;
import edu.cmu.sphinx.frontend.util.AudioFileDataSource;
import edu.cmu.sphinx.util.props.ConfigurationManager;


public class FrontendElementTest {

    @DataProvider(name = "frontendProvider")
    public Object[][] provide() {
        return new Object[][] {
            {
                "preempTest",
                "after-preemp.dump"},
            {
                "windowTest",
                "after-window.dump"},
            {
                "fftTest",
                "after-fft.dump"},
            {
                "melTest",
                "after-mel.dump"},
            {
                "dctTest",
                "after-dct.dump"},
            {
                "cmnTest",
                "after-cmn.dump"},
            {
                "feTest",
                "after-feature.dump"}};
    }

    @Test(dataProvider = "frontendProvider")
    public void testElement(String frontendName, String name)
            throws IOException {
        URL url = getClass().getResource("frontend.xml");
        ConfigurationManager cm = new ConfigurationManager(url);

        AudioFileDataSource ds = cm.lookup("audioFileDataSource");
        ds.setAudioFile(getClass().getResource("test-feat.wav"), null);

        FrontEnd frontend = cm.lookup(frontendName);
        compareDump(frontend, name);
    }

    private void compareDump(FrontEnd frontend, String name)
            throws NumberFormatException, DataProcessingException, IOException {
        InputStream stream = getClass().getResource(name).openStream();
        Reader reader = new InputStreamReader(stream);
        BufferedReader br = new BufferedReader(reader);
        String line;
        
        while (null != (line = br.readLine())) {
            Data data = frontend.getData();

            if (line.startsWith("DataStartSignal"))
                assertThat(data, instanceOf(DataStartSignal.class));
            if (line.startsWith("DataEndSignal"))
                assertThat(data, instanceOf(DataEndSignal.class));
            if (line.startsWith("SpeechStartSignal"))
                assertThat(data, instanceOf(SpeechStartSignal.class));
            if (line.startsWith("SpeechEndSignal"))
                assertThat(data, instanceOf(SpeechEndSignal.class));

            if (line.startsWith("Frame")) {
                assertThat(data, instanceOf(DoubleData.class));

                double[] values = ((DoubleData) data).getValues();
                String[] tokens = line.split(" ");

                assertThat(values.length, equalTo(parseInt(tokens[1])));

                for (int i = 0; i < values.length; i++)
                    assertThat(values[i],
                               closeTo(parseDouble(tokens[2 + i]),
                                       abs(0.01 * values[i])));
            }

            if (line.startsWith("FloatFrame")) {
                String[] tokens = line.split(" ");
                Assert.assertTrue(data instanceof FloatData);
                float[] values = ((FloatData) data).getValues();
                Assert.assertEquals(values.length,
                                    (int) Integer.valueOf(tokens[1]));
                for (int i = 0; i < values.length; i++)
                    assertThat(Double.valueOf(values[i]),
                               closeTo(parseFloat(tokens[2 + i]),
                                       abs(0.01 * values[i])));
            }
        }
    }
}
