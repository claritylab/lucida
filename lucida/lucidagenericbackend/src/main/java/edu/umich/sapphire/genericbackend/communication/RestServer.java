package edu.umich.sapphire.genericbackend.communication;

import edu.umich.sapphire.genericbackend.orchestration.Controller;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.glassfish.jersey.server.ResourceConfig;
import org.glassfish.jersey.servlet.ServletContainer;

/**
 * Created by Falk Pollok
 */
public class RestServer {
    private final static int REST_SERVICE_PORT = 9091;
    private Controller controller;

    public void start(Controller controller) throws Exception {
        this.controller = controller;

        Server server = new Server(REST_SERVICE_PORT);

        //UrlManager.lookUpUrl(UrlManager.Backends.BLUEMIX.ordinal());

        ServletContextHandler servletContextHandler = new ServletContextHandler();
        servletContextHandler.setContextPath("");
        servletContextHandler.addServlet(new ServletHolder(new ServletContainer(resourceConfig(controller))), "/*");
        server.setHandler(servletContextHandler);

        try {
            server.start();
            server.join();
        } finally {
            server.destroy();
        }
    }

    private ResourceConfig resourceConfig(Controller controller) {
        return new ResourceConfig().register(new RestHandler(controller));
    }

}
