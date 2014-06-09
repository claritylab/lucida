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

import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;

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
        
//        String timeStamp = new SimpleDateFormat(" mm ss S").format(new Date( ));

        InputStream in = (InputStream) request.getInputStream();
        
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        int read = 0;
        byte[] bytes = new byte[4096];
        while ((read = in.read(bytes)) != -1) {
           //  fos.write(bytes, 0, read);   
             bos.write(bytes, 0, read);                
        }
//        fos.close();          
        bos.close();                  
        in.close(); 
        
        byte[] data = bos.toByteArray();
        ByteArrayInputStream bin = new ByteArrayInputStream(data);
        
        text = decode_voice_sphinx4(bin);

        // response
        response.setContentType("text/html;charset=utf-8");
        response.setStatus(HttpServletResponse.SC_OK);
        baseRequest.setHandled(true);

        if (text != null)
                response.getWriter().println("You said: " + text);
        else
                response.getWriter().println("Coudn't decode/understand your voice...");        
    }
          
    

    protected String decode_voice_sphinx4(ByteArrayInputStream bin) {

        String result_text = null;

       final String conf_file = System.getenv("CONF_FILE");

        ConfigurationManager sphinx4_config = new ConfigurationManager(conf_file);

//		MsgPrinter.printStatusMsg("created config manager");
        try {
            StreamDataSource streamDataSource = (StreamDataSource) sphinx4_config.lookup("streamDataSource");
            //streamDataSource.setInputStream(bin, "Main Stream");
            streamDataSource.setInputStream(bin);

        } catch (Exception e) {
            e.printStackTrace();
        }

//		MsgPrinter.printStatusMsg("read input stream");
        Recognizer recognizer = (Recognizer) sphinx4_config.lookup("recognizer");

//		MsgPrinter.printStatusMsg("lookup recognize");		
        synchronized (this) {

            recognizer.allocate();

        }

//		MsgPrinter.printStatusMsg("allocate recognizer, now will recognize");		
        try {

            Result result = recognizer.recognize();
            result_text = result.getBestResultNoFiller();
            System.out.println("result_text: " + result_text);

        } catch (Exception e) {
            e.printStackTrace();
        }

//		MsgPrinter.printStatusMsg("done recognized");                                                               
        // recognizer.resetMonitors();
        // recognizer.deallocate();                                           
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
        Server server = new Server(8080);
        server.setHandler(new Sphinx4Server()); 
 
        server.start();
        server.join();
    }
}
