package lucida.main;

// Thrift java libraries 

import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TNonblockingServerTransport;
import org.apache.thrift.server.TThreadedSelectorServer;
import org.apache.thrift.transport.TTransportException;

import java.io.IOException;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Properties;

import org.apache.thrift.TProcessor;
// Thrift client-side code for registering w/ sirius
import org.apache.thrift.protocol.TBinaryProtocol;

// Generated code
import lucida.thrift.*;

// The service handler
import lucida.handler.*;
import lucida.handler.QAServiceHandler.AsyncQAServiceHandler;

/**
 * Starts the question-answer server and listens for requests.
 */
public class QADaemon {
    /**
     * Entry point for question-answer.
     *
     * @param args the argument list. Provide port numbers
     *             for both Lucida and QA.
     */
    public static void main(String[] args)
            throws TTransportException, IOException, InterruptedException {
        Properties port_cfg = new Properties();
        InputStream input = new FileInputStream("../../config.properties");
        port_cfg.load(input);
        String port_str = port_cfg.getProperty("QA_PORT");
        Integer port = Integer.valueOf(port_str);
        TProcessor proc = new LucidaService.AsyncProcessor(new QAServiceHandler.AsyncQAServiceHandler());
        TNonblockingServerTransport transport = new TNonblockingServerSocket(port);
        TThreadedSelectorServer.Args arguments = new TThreadedSelectorServer.Args(transport)
                .processor(proc)
                .protocolFactory(new TBinaryProtocol.Factory())
                .transportFactory(new TFramedTransport.Factory());
        final TThreadedSelectorServer server = new TThreadedSelectorServer(arguments);
        System.out.println("QA at port " + port_str);
        server.serve();
    }
}
