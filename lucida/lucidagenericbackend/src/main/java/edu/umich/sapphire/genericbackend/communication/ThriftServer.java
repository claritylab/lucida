package edu.umich.sapphire.genericbackend.communication;

import edu.umich.sapphire.genericbackend.orchestration.Controller;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TSimpleServer;
import org.apache.thrift.transport.TServerSocket;
import org.apache.thrift.transport.TServerTransport;

/**
 * Created by fp on 5/13/16.
 */
public class ThriftServer {

    public static ThriftHandler thriftHandler;
    public static LucidaService.Processor lucidaServiceProcessor;

    public static void setup(Controller controller) {
        thriftHandler = new ThriftHandler(controller);
        lucidaServiceProcessor = new LucidaService.Processor(thriftHandler);
        System.out.println("Created handler and processor.");

        Runnable simple = new Runnable() {
            public void run() { simple(lucidaServiceProcessor); }
        };
        new Thread(simple).start();
    }

    public static void simple(LucidaService.Processor lucidaServiceProcessor) {
        try {
            TServerTransport serverTransport = new TServerSocket(9092);
            TServer server = new TSimpleServer(new TServer.Args(serverTransport).processor(lucidaServiceProcessor));
            System.out.println("Starting server...");
            server.serve();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
