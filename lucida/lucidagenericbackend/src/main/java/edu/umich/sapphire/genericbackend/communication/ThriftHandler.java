package edu.umich.sapphire.genericbackend.communication;

import edu.umich.sapphire.genericbackend.orchestration.Controller;
import org.apache.thrift.TException;

/**
 *
 */
public class ThriftHandler extends Handler implements LucidaService.Iface {

    Controller controller;
    public ThriftHandler(Controller controller) {
        this.controller = controller;
    }

    @Override
    public void create(String LUCID, QuerySpec spec) throws TException {
        System.out.println("Create");
    }

    @Override
    public void learn(String LUCID, QuerySpec knowledge) throws TException {
        System.out.println("learn");
    }

    @Override
    public String infer(String LUCID, QuerySpec query) throws TException {
        System.out.println("Received question: " + LUCID);
        //return controller.getQaPipe().ask(LUCID);
        return "No";
    }
}
