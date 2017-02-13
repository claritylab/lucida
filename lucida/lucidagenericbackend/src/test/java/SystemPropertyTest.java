import edu.umich.sapphire.genericbackend.communication.UrlManager;
import org.testng.Assert;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Test;


/**
 * Verifies the default ports and overwriting them.
 *
 * TODO Currently does not test against wrong input, e.g. strings
 */
public class SystemPropertyTest {

    @Test()
    public void testSystemProperties() {
        // Test REST port with and without system property
        Assert.assertEquals(Integer.parseInt(UrlManager.lookUpUrl(UrlManager.Ports.REST.ordinal())), 9091);

        System.setProperty("edu.umich.sapphire.genericbackend.restport", "1234");
        Assert.assertEquals(Integer.parseInt(System.getProperty("edu.umich.sapphire.genericbackend.restport")), 1234);
        Assert.assertEquals(Integer.parseInt(UrlManager.lookUpUrl(UrlManager.Ports.REST.ordinal())), 1234);

        // Test Thrift port with and withour system property
        Assert.assertEquals(Integer.parseInt(UrlManager.lookUpUrl(UrlManager.Ports.THRIFT.ordinal())), 9090);

        System.setProperty("edu.umich.sapphire.genericbackend.thriftport", "2345");
        Assert.assertEquals(Integer.parseInt(System.getProperty("edu.umich.sapphire.genericbackend.thriftport")), 2345);
        Assert.assertEquals(Integer.parseInt(UrlManager.lookUpUrl(UrlManager.Ports.THRIFT.ordinal())), 2345);
    }

}
