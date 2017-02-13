import edu.umich.sapphire.genericbackend.communication.RestServer;
import edu.umich.sapphire.genericbackend.communication.ThriftServer;
import edu.umich.sapphire.genericbackend.communication.UrlManager;
import edu.umich.sapphire.genericbackend.orchestration.Controller;
import org.testng.Assert;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Test;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;

import static java.lang.Thread.sleep;

/**
 * This test probes the REST interface.
 *
 * Note: This test is slow, since it needs to start a server first. For most tests it should be fine to test the
 * respective method directly.
 */
public class RestTest {

    private static Controller controller;

    @BeforeSuite
    public void beforeTest() {
        // Create controller and run REST server as thread
        controller = new Controller();
        Thread threadRest = new Thread(new Runnable() {
            public void run() {
                try {
                    new RestServer().start(controller);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
        threadRest.start();

        // Wait for 2s so Jersey can start
        try {
            sleep(2000);
        } catch (InterruptedException ie) {
            ie.printStackTrace();
        }
    }

    @Test()
    public void testRest() {
        // Open example URL and see whether the output is correct
        try {
            URL url = new URL("http://127.0.0.1:9091/backend/function?parameter=input231");
            URLConnection urlConnection = url.openConnection();
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(urlConnection.getInputStream()));
            String line;
            StringBuilder content = new StringBuilder();
            while ((line = bufferedReader.readLine()) != null) {
                content.append(line);
            }
            bufferedReader.close();
            Assert.assertEquals(content.toString(),"input231");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
