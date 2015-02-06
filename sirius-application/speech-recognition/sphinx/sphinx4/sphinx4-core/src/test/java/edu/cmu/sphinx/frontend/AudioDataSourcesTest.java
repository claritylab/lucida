package edu.cmu.sphinx.frontend;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.instanceOf;
import static org.testng.Assert.assertFalse;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.URISyntaxException;

import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.util.AudioFileDataSource;
import edu.cmu.sphinx.frontend.util.AudioFileProcessListener;
import edu.cmu.sphinx.frontend.util.ConcatAudioFileDataSource;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;


/**
 * Some small unit tests to check whether the AudioFileDataSource and the
 * ConcatAudioFileDataSource are working properly.
 */
public class AudioDataSourcesTest {

    private int numFileStarts;
    private int numFileEnds;

    @BeforeMethod
    public void setUp() {
        numFileStarts = 0;
        numFileEnds = 0;
    }

    @Test
    public void testSimpleFileSources() throws DataProcessingException,
            URISyntaxException {
        // TODO: test.ogg
        runAssert("test.wav");
        runAssert("test.aiff");
        runAssert("test.au");
    }

    @Test
    public void test8KhzSource() throws DataProcessingException,
            URISyntaxException {
        AudioFileDataSource dataSource = ConfigurationManager
                .getInstance(AudioFileDataSource.class);

        // Test simple WAV.
        File file = new File(getClass().getResource("test8k.wav").toURI());
        dataSource.setAudioFile(file, null);
        assertThat(dataSource.getData(), instanceOf(DataStartSignal.class));
        Data d = dataSource.getData();
        assertThat(dataSource.getData(), instanceOf(DoubleData.class));
        assertThat(((DoubleData) d).getSampleRate(), equalTo(8000));

        while ((d = dataSource.getData()) instanceof DoubleData);
        assertThat(d, instanceOf(DataEndSignal.class));
    }

    @Test
    public void testConcatDataSource() throws DataProcessingException,
            IOException, URISyntaxException {
        ConcatAudioFileDataSource dataSource = ConfigurationManager
                .getInstance(ConcatAudioFileDataSource.class);

        dataSource.addNewFileListener(new AudioFileProcessListener() {

            public void audioFileProcStarted(File audioFile) {
                numFileStarts++;
            }

            public void audioFileProcFinished(File audioFile) {
                numFileEnds++;
            }

            public void newProperties(PropertySheet ps)
                    throws PropertyException {
            }
        });

        File tmpFile = File.createTempFile(getClass().getName(), ".drv");
        tmpFile.deleteOnExit();
        PrintWriter pw = new PrintWriter(tmpFile);
        String path = new File(getClass().getResource("test.wav").toURI()).getPath();
        pw.println(path);
        pw.println(path);
        pw.print(path);
        assertFalse(pw.checkError());
        pw.close();

        dataSource.setBatchFile(tmpFile);
        assertThat(dataSource.getData(), instanceOf(DataStartSignal.class));
        assertThat(dataSource.getData(), instanceOf(DoubleData.class));

        Data d;
        while ((d = dataSource.getData()) instanceof DoubleData);
        assertThat(d, instanceOf(DataEndSignal.class));

        assertThat(numFileStarts, equalTo(3));
        assertThat(numFileEnds, equalTo(3));
    }

    private void runAssert(String fileName) throws DataProcessingException,
            URISyntaxException {
        AudioFileDataSource dataSource = ConfigurationManager
                .getInstance(AudioFileDataSource.class);

        File file = new File(getClass().getResource(fileName).toURI());
        dataSource.setAudioFile(file, null);
        assertThat(dataSource.getData(), instanceOf(DataStartSignal.class));
        assertThat(dataSource.getData(), instanceOf(DoubleData.class));

        Data d;
        while ((d = dataSource.getData()) instanceof DoubleData);
        assertThat(d, instanceOf(DataEndSignal.class));
    }
}
