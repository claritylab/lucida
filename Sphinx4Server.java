//
//  ========================================================================
//  Copyright (c) 1995-2014 Mort Bay Consulting Pty. Ltd.
//  ------------------------------------------------------------------------
//  All rights reserved. This program and the accompanying materials
//  are made available under the terms of the Eclipse Public License v1.0
//  and Apache License v2.0 which accompanies this distribution.
//
//      The Eclipse Public License is available at
//      http://www.eclipse.org/legal/epl-v10.html
//
//      The Apache License v2.0 is available at
//      http://www.opensource.org/licenses/apache2.0.php
//
//  You may elect to redistribute this code under either of these licenses.
//  ========================================================================
//

import java.io.*;

// audio stuff
import javax.sound.sampled.*;
import java.nio.*;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.nio.SelectChannelConnector;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;
import org.eclipse.jetty.util.thread.QueuedThreadPool;

import java.net.URL;
import javax.sound.sampled.*;
import java.nio.*;

import java.lang.Runtime.*;
import java.io.InputStreamReader;

/* sphinx4 */
import edu.cmu.sphinx.frontend.util.AudioFileDataSource;
import edu.cmu.sphinx.frontend.util.StreamDataSource;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.props.ConfigurationManager;

public class Sphinx4Server extends AbstractHandler
{
    @Override
    public void handle(String target,
                       Request baseRequest,
                       HttpServletRequest request,
                       HttpServletResponse response) 
        throws IOException, ServletException
    {        
            
        String text = null;
        InputStream in = (InputStream) request.getInputStream();
        
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        int read = 0;
        byte[] bytes = new byte[4096];
        while ((read = in.read(bytes)) != -1) {
             bos.write(bytes, 0, read);                
        }
        bos.close();                  
        in.close(); 
        
        byte[] data = bos.toByteArray();
        ByteArrayInputStream bin = new ByteArrayInputStream(data);
        
        text = decode_voice_sphinx4(bin);

        // response
        response.setContentType("text/html;charset=utf-8");
        response.setStatus(HttpServletResponse.SC_OK);
        baseRequest.setHandled(true);

        // send response
        if (text != null)
                response.getWriter().println(text);
        else
                response.getWriter().println("(NULL)");        
    }

    protected String decode_voice_sphinx4(ByteArrayInputStream bin) {

        String result_text = null;

       final String conf_file = System.getenv("CONF_FILE");

        ConfigurationManager sphinx4_config = new ConfigurationManager(conf_file);

        try {
            StreamDataSource streamDataSource = (StreamDataSource) sphinx4_config.lookup("streamDataSource");
            streamDataSource.setInputStream(bin);

        } catch (Exception e) {
            e.printStackTrace();
        }

        Recognizer recognizer = (Recognizer) sphinx4_config.lookup("recognizer");

        synchronized (this) {
            recognizer.allocate();
        }

        try {

            Result result = recognizer.recognize();
            result_text = result.getBestResultNoFiller();
            System.out.println("result_text: " + result_text);

        } catch (Exception e) {
            e.printStackTrace();
        }

        synchronized (this) {
            recognizer.deallocate();
        }

        if (result_text == null) {
            result_text = "None";
        }

        return result_text;
    }

    
    public static void main(String[] args) throws Exception
    {
        String addr = args[0];
        int port = Integer.parseInt(args[1]);
        int NTHREADS = Integer.parseInt(System.getenv("THREADS"));

        Server server = new Server();
        SelectChannelConnector con1 = new SelectChannelConnector();
        con1.setHost(addr);
        con1.setPort(port);
        con1.setThreadPool(new QueuedThreadPool(NTHREADS));
        con1.setMaxIdleTime(30000);
        con1.setRequestHeaderSize(8192);

        server.setConnectors(new Connector[]{con1});
        server.setHandler(new Sphinx4Server()); 
 
        server.start();
        server.join();
    }
}
