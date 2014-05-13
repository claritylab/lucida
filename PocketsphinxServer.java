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

/* pocketsphinx */
import edu.cmu.pocketsphinx.Decoder;
import edu.cmu.pocketsphinx.Config;
import edu.cmu.pocketsphinx.Hypothesis;

/* sphinx4 */

public class PocketsphinxServer extends AbstractHandler
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
        
        text = decode_pocketsphinx(bin);

        // response
        response.setContentType("text/html;charset=utf-8");
        response.setStatus(HttpServletResponse.SC_OK);
        baseRequest.setHandled(true);

        if (text != null)
                response.getWriter().println("You said: " + text);
        else
                response.getWriter().println("Coudn't decode/understand your voice...");        
    }
          
    
      static {
           System.loadLibrary("pocketsphinx_jni");
      }        
      
      public String decode_pocketsphinx(ByteArrayInputStream bin) {

                final String path = System.getenv("MODELS_PATH");
      
                Config sphinx_config = Decoder.defaultConfig();
                sphinx_config.setString("-dict", path + "/cmu07a.dic");
                sphinx_config.setString("-hmm", path + "/voxforge_en_sphinx.cd_cont_5000/");
                sphinx_config.setString("-lm", path + "lm_giga_64k_nvp_3gram.lm.DMP");
                Decoder sphinx_decoder = new Decoder(sphinx_config);	      
                                

                AudioInputStream ais = null;
                try {
                    AudioInputStream tmp = AudioSystem.getAudioInputStream(bin);
                    // Convert it to the desired audio format for PocketSphinx. 
                    //MsgPrinter.printErrorMsg("samprate: " + (float)c.getFloat("-samprate"));
                    AudioFormat targetAudioFormat = new AudioFormat(16000, 16, 1, true, true);
                    ais = AudioSystem.getAudioInputStream(targetAudioFormat, tmp);
                } catch (IOException e) {
                    System.out.println("Failed to open " + e.getMessage());
                } catch (UnsupportedAudioFileException e) {
                    System.out.println("Unsupported file type of " + e.getMessage());
                }             
                sphinx_decoder.startUtt(null);
                byte[] b = new byte[4096];
                try {
                    int nbytes;
                    while ((nbytes = ais.read(b)) >= 0) {
                        ByteBuffer bb = ByteBuffer.wrap(b, 0, nbytes);
                        short[] s = new short[nbytes/2];
                        bb.asShortBuffer().get(s);
                        sphinx_decoder.processRaw(s, s.length, false, false);
                    }
                } catch (IOException e) {
                    System.out.println("Error when reading wav file" + e.getMessage());
                }
                sphinx_decoder.endUtt();        
                
                Hypothesis h = sphinx_decoder.hyp();    
                if (h != null) {
                    return h.getHypstr();
                }

		return "";      
      
      }             
    
    public static void main(String[] args) throws Exception
    {
        Server server = new Server(8080);
        server.setHandler(new PocketsphinxServer()); 
 
        server.start();
        server.join();
    }
}
