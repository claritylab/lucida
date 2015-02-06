/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.util;


import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

/**
 * This class is a command interpreter. It reads strings from an input stream, parses them into commands and executes
 * them, results are sent back on the output stream.
 *
 * @see CommandInterpreter
 */

public class SocketCommandInterpreter extends Thread {

    private final int port;
    private final Map<String, CommandInterface> commandList;
    private boolean trace;
    private boolean acceptConnections = true;


    /**
     * Creates a command interpreter at the given port.
     *
     * @param port the port to create the command interpreter
     */
    public SocketCommandInterpreter(int port) {
        this.port = port;
        commandList = new HashMap<String, CommandInterface>();
    }


    /**
     * Adds the given command to the command list.
     *
     * @param name    the name of the command.
     * @param command the command to be executed.
     */

    public void add(String name, CommandInterface command) {
        commandList.put(name, command);
    }

    // Implements the run() method of Runnable


    @Override
    public final void run() {
        ServerSocket ss;

        try {
            ss = new ServerSocket(port);
            System.out.println("Waiting on " + ss);
        } catch (IOException ioe) {
            System.out.println("Can't open socket on port " + port);
            ioe.printStackTrace();
            return;
        }

        while (acceptConnections) {
            try {
                Socket s = ss.accept();
                spawnCommandInterpreter(s);
            } catch (IOException ioe) {
                System.err.println("Could not accept socket " + ioe);
                ioe.printStackTrace();
                break;
            }
        }

        try {
            ss.close();
        } catch (IOException ioe) {
            System.err.println("Could not close server socket " + ioe);
            ioe.printStackTrace();
        }
    }


    /** Stops this SocketCommandInterpreter from accepting connections. Effectively stops this thread. */
    public void setStopAcceptConnections() {
        acceptConnections = false;
    }


    /**
     * Sets the trace mode of the command interpreter.
     *
     * @param trace true if tracing.
     */

    public void setTrace(boolean trace) {
        this.trace = trace;
    }


    /**
     * spawns a command interpreter
     *
     * @param    s the socket where the interpeter reads/writes to/from
     */
    private void spawnCommandInterpreter(Socket s) {
        try {
            BufferedReader inReader = new BufferedReader(
                    new InputStreamReader(s.getInputStream()));
            PrintWriter outWriter = new PrintWriter(
                    s.getOutputStream(), true);
            CommandInterpreter ci = new CommandInterpreter(inReader, outWriter);
            ci.setSocket(s);
            ci.add(commandList);
            ci.setTrace(trace);
            ci.start();
        } catch (IOException ioe) {
            System.err.println("Could not attach CI to socket " + ioe);
        }
    }


    /** manual tester for the command interpreter. */

    public static void main(String[] args) {
        SocketCommandInterpreter sci = new SocketCommandInterpreter(7890);
        sci.add("testCommand", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                return "this is a test";
            }


            public String getHelp() {
                return "a test command";
            }
        });
        sci.add("time", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                return "Time is " + new Date();
            }


            public String getHelp() {
                return "shows the current time";
            }
        });

        System.out.println("Welcome to SocketCommand interpreter test program");
        sci.setTrace(true);
        sci.start();
    }
}
