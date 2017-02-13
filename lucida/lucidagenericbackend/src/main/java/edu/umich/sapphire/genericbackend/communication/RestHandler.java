package edu.umich.sapphire.genericbackend.communication;

import edu.umich.sapphire.genericbackend.orchestration.Controller;

import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.MediaType;

/**
 * Created by Falk Pollok
 */
@Path("backend")
public class RestHandler {

    private Controller controller;

    public RestHandler(Controller controller) {
        this.controller = controller;
    }

    @GET
    @Path("function")
    @Produces(MediaType.TEXT_PLAIN)
    public String ask(@QueryParam("parameter") String question) {
        // http://localhost:9091/backend/function?parameter=input
        return controller.getQaPipe().ask(question);
    }

}
