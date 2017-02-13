package edu.umich.sapphire.genericbackend;

import edu.umich.sapphire.genericbackend.communication.ThriftAsyncServer;
import edu.umich.sapphire.genericbackend.communication.ThriftServer;
import edu.umich.sapphire.genericbackend.communication.RestServer;
import edu.umich.sapphire.genericbackend.orchestration.Controller;

/**
 * This Main class creates an ThriftServer that uses an ThriftHandler to query the ensemble
 * The handler asks the Dispatcher for an answer, which collects answers from all backends that are afterwards
 * merged by the ListMerger using a specific Strategy like a MajorityVote
 * Finally, all question/answer sets are persisted by the Database Manager
 *
 * Alternatively, a Jetty-based RESTful interface can be used
 *
 * To use: "docker-compose up" to start the data backends, web qa and YodaQA, then launch this ensemble
 * Query via Python client, which sends a question and displays an answer
 * Soon this ensemble will also be usable directly from Lucida
 *
 */
public class Main {
    //private static RestServer restServer;
    //private static boolean launchRest = false;

    private static Controller controller;

    public static void main(String[] args) throws Exception {

        controller = new Controller();

//        System.out.println("Command line arguments:");
//        for (String s: args) {
//            System.out.println(s);
//            if(s.toLowerCase().equals("rest")) {
//                launchRest = true;
//            }
//        }
//        if(launchRest) {
//            restServer = new RestServer();
//            restServer.start(controller);
//        }

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

//        Thread threadThrift = new Thread(new Runnable() {
//            public void run() {
//                ThriftServer.setup(controller);
//            }
//        });
//        threadThrift.start();

        Thread threadThrift = new Thread(new Runnable() {
            public void run() {
                ThriftAsyncServer.setup(controller);
            }
        });
        threadThrift.start();


        //ThriftServer.setup(controller);
    }
}
