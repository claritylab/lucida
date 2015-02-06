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
import java.net.Socket;
import java.net.SocketException;

/**
 * A client for a socketed command interpreter. Hooks up to a socketed command interpreter and allows the sending of
 * commands.
 *
 * @see CommandInterpreter
 */

public class SocketCommandClient {

    private String host;
    private int port;
    private Socket socket;
    private BufferedReader inReader;
    private PrintWriter outWriter;


    /**
     * Creates a socket command client at the given host and port.
     *
     * @param host the host machine
     * @param port the port to use
     */
    public SocketCommandClient(String host, int port)
            throws IOException {
        this.host = host;
        this.port = port;
        open();
    }


    /** Creats a SocketCommandClient with no connection open must be called. */
    public SocketCommandClient() {
    }


    /**
     * Opens a socket connection
     *
     * @param aHost the host to connect to
     * @param aPort the port to connect to
     * @throws IOException if connection fails
     */
    public synchronized void open(String aHost, int aPort)
            throws IOException {
        host = aHost;
        port = aPort;

        /* Open a client socket connection, throws IOExceptions
       * to the calling thread.
       * RATIONALE: the caller should handle any connection-
       * related errors.
       */
        socket = new Socket(host, port);

        inReader = new BufferedReader
                (new InputStreamReader(socket.getInputStream()));
        outWriter = new PrintWriter(socket.getOutputStream(), true);
    }


    private synchronized void open() throws IOException {
        open(host, port);
    }


    /**
     * Returns the SO_TIMEOUT of the Socket that this client uses. 0 returns implies that the option is disabled (i.e.,
     * timeout of infinity).
     */
    public int getSoTimeout() throws SocketException {
        if (socket != null) {
            return socket.getSoTimeout();
        } else {
            return 0;
        }
    }


    /**
     * Enable/disable SO_TIMEOUT with the specified timeout, in milliseconds. The timeout must be > 0. A timeout of zero
     * is interpreted as an infinite timeout.
     *
     * @param millisecs the timeout in milliseconds
     * @throws SocketException
     */
    public void setSoTimeout(int millisecs) throws SocketException {
        if (socket != null) {
            socket.setSoTimeout(millisecs);
        } else {
            System.err.println("SocketCommandClient.setSoTimeout(): " +
                    "socket is null");
        }
    }


    /**
     * sends a command, retries on error which will attempt to repair a dead socket
     *
     * @param command the command
     * @return true if command was sent ok
     */

    public synchronized boolean sendCommand(String command) {
        final int maxTries = 2;

        for (int i = 0; i < maxTries; i++) {
            if (!checkOpen()) {
                continue;
            }
            outWriter.println(command);
            if (outWriter.checkError()) {
                close();
                System.err.println("IO error while sending " + command);
            } else {
                return true;
            }
        }
        return false;
    }


    /**
     * Gets a response
     *
     * @return the response or null if error
     */

    public synchronized String getResponse() {
        String response = null;

        if (!checkOpen()) {
            return null;
        }

        try {
            response = inReader.readLine();
        } catch (IOException ioe) {
            System.err.println("IO error while reading response");
            close();
        }
        return response;
    }


    /**
     * is a response
     *
     * @return the response or null if error
     */

    public synchronized boolean isResponse() {
        boolean response = false;

        if (!checkOpen()) {
            return false;
        }

        try {
            response = inReader.ready();
        } catch (IOException ioe) {
            System.err.println("IO error while checking response");
            close();
        }
        return response;
    }


    /**
     * sends a command get a response
     *
     * @param command the command to send
     * @return the response or null if error
     */

    public synchronized String sendCommandGetResponse(String command) {
        String response = null;
        if (sendCommand(command)) {
            response = getResponse();
        }
        return response;
    }


    /** Closes the socket connection */
    public synchronized void close() {
        try {
            if (socket != null) {
                socket.close();
            } else {
                System.err.println("SocketCommandClient.close(): " +
                        "socket is null");
            }
        } catch (IOException ioe) {
            System.err.println("Trouble closing socket");
        }
        socket = null;
    }


    /** Checks to see if the socket is open, if not opens it. */
    private synchronized boolean checkOpen() {
        try {
            if (socket == null)
                open();
        }
        catch (IOException e) {
            System.err.println("SocketCommandClient.checkOpen():"
                    + "could not open socket");
            socket = null;
        }
        return socket != null;
    }


    /** manual tester for the command interpreter. */

    public static void main(String[] args) {
        try {
            CommandInterpreter ci = new CommandInterpreter();
            final SocketCommandClient sci = new
                    SocketCommandClient("localhost", 7890);
            ci.add("s", new CommandInterface() {
                public String execute(CommandInterpreter ci, String[] args) {
                    StringBuilder cmd = new StringBuilder();
                    for (int i = 1; i < args.length; i++) {
                        cmd.append(args[i]).append(' ');
                    }
                    sci.sendCommand(cmd.toString());
                    return "";
                }


                public String getHelp() {
                    return "send a command";
                }
            });

            ci.add("r", new CommandInterface() {
                public String execute(CommandInterpreter ci, String[] args) {
                    while (sci.isResponse()) {
                        ci.putResponse(sci.getResponse());
                    }
                    return "";
                }


                public String getHelp() {
                    return "receive a response";
                }
            });

            ci.add("sr", new CommandInterface() {
                public String execute(CommandInterpreter ci, String[] args) {
                    StringBuilder cmd = new StringBuilder();
                    for (int i = 1; i < args.length; i++) {
                        cmd.append(args[i]).append(' ');
                    }
                    ci.putResponse(sci.sendCommandGetResponse(cmd.toString()));
                    while (sci.isResponse()) {
                        ci.putResponse(sci.getResponse());
                    }
                    return "";
                }


                public String getHelp() {
                    return "send a command, receive a response";
                }
            });

            ci.setPrompt("scc-test> ");
            ci.run();
        }
        catch (Exception e) {
            System.err.println("error occured.");
            e.printStackTrace();
            System.exit(-1);
        }
    }
}
