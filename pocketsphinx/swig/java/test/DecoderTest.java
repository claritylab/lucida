package test;

import static org.junit.Assert.*;

import org.junit.Test;

import java.io.IOException;
import java.net.URL;
import javax.sound.sampled.*;
import java.nio.*;

import edu.cmu.pocketsphinx.Decoder;
import edu.cmu.pocketsphinx.Config;
import edu.cmu.pocketsphinx.Hypothesis;

public class DecoderTest {
    static {
        System.loadLibrary("pocketsphinx_jni");
    }

    @Test
    public void testDecoder() {
        Decoder d = new Decoder();
        assertNotNull(d);
    }

    @Test
    public void testDecoderConfig() {
        Config c = Decoder.default_config();
        c.set_boolean("-backtrace", true);
        Decoder d = new Decoder(c);
        assertNotNull(d);
    }

    @Test
    public void testGetConfig() {
        Decoder d = new Decoder();
        Config c = d.get_config();
        assertNotNull(c);
        assertEquals(c.get_string("-lmname"), "default");
    }

    @Test
    public void testDecodeRaw() {
        Config c = Decoder.default_config();
        c.set_float("-samprate", 8000);
        Decoder d = new Decoder(c);
        AudioInputStream ais = null;
        try {
            URL testwav = new URL("file:../../test/data/wsj/n800_440c0207.wav");
            AudioInputStream tmp = AudioSystem.getAudioInputStream(testwav);
            /* Convert it to the desired audio format for PocketSphinx. */
            AudioFormat targetAudioFormat =
                new AudioFormat((float)c.get_float("-samprate"),
                                16, 1, true, true);
            ais = AudioSystem.getAudioInputStream(targetAudioFormat, tmp);
        } catch (IOException e) {
            fail("Failed to open " + e.getMessage());
        } catch (UnsupportedAudioFileException e) {
            fail("Unsupported file type of " + e.getMessage());
        }

        d.start_utt("");
        byte[] b = new byte[4096];
        try {
            int nbytes;
            while ((nbytes = ais.read(b)) >= 0) {
                ByteBuffer bb = ByteBuffer.wrap(b, 0, nbytes);
                short[] s = new short[nbytes/2];
                bb.asShortBuffer().get(s);
                d.process_raw(s, false, false);
                System.out.println(d.hyp().getHypstr());
            }
        } catch (IOException e) {
            fail("Error when reading goforward.wav" + e.getMessage());
        }
        d.end_utt();
    }
}
