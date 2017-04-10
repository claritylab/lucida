import edu.umich.sapphire.genericbackend.communication.RestHandler;
import edu.umich.sapphire.genericbackend.communication.RestServer;
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
 * This test verifies methods directly rather than using a network interface, which would be slower.
 */
public class RestHandlerTest {

    private RestHandler restHandler;

    @BeforeSuite
    public void beforeTest() {
        restHandler = new RestHandler(new Controller());
    }

    @Test()
    public void testRestHandler() {
        Assert.assertEquals(restHandler.ask("CrossLinguisticOnomatopoeias"),"CrossLinguisticOnomatopoeias");
    }
}
