# -*- coding: UTF-8 -*-

'''
Created on Jun 27, 2013

@author: tanel
'''
import unittest
from gi.repository import GObject, Gst
import thread
import logging
from decoder import DecoderPipeline
import time

class DecoderPipelineTests(unittest.TestCase):

    def __init__(self,  *args, **kwargs):
        super(DecoderPipelineTests, self).__init__(*args, **kwargs)
        logging.basicConfig(level=logging.INFO)

    @classmethod
    def setUpClass(cls):
            decoder_conf = {"model" : "test/models/estonian/tri2b_mmi_pruned/final.mdl",
                            "lda-mat" : "test/models/estonian/tri2b_mmi_pruned/final.mat",
                            "word-syms" : "test/models/estonian/tri2b_mmi_pruned/words.txt",
                            "fst" : "test/models/estonian/tri2b_mmi_pruned/HCLG.fst",
                            "silence-phones" : "6"}
            cls.decoder_pipeline = DecoderPipeline({"decoder" : decoder_conf})
            cls.words = []
            cls.finished = False

            cls.decoder_pipeline.set_word_handler(cls.word_getter)
            cls.decoder_pipeline.set_eos_handler(cls.set_finished, cls.finished)

            loop = GObject.MainLoop()
            thread.start_new_thread(loop.run, ())

    @classmethod
    def word_getter(cls, word):
        cls.words.append(word)

    @classmethod
    def set_finished(cls, finished):
        cls.finished = True

    def setUp(self):
        self.__class__.words = []
        self.__class__.finished = False



    def testCancelAfterEOS(self):
        self.decoder_pipeline.init_request("testCancelAfterEOS", "audio/x-raw, layout=(string)interleaved, rate=(int)16000, format=(string)S16LE, channels=(int)1")
        f = open("test/data/1234-5678.raw", "rb")
        for block in iter(lambda: f.read(8000), ""):
            time.sleep(0.25)
            self.decoder_pipeline.process_data(block)

        self.decoder_pipeline.end_request()
        self.decoder_pipeline.cancel()
        while not self.finished:
            time.sleep(1)

        #self.assertEqual(["üks", "kaks", "kolm", "neli", "<#s>", "viis", "kuus", "seitse", "kaheksa", "<#s>"], self.words)


    def test12345678(self):
        self.decoder_pipeline.init_request("test12345678", "audio/x-raw, layout=(string)interleaved, rate=(int)16000, format=(string)S16LE, channels=(int)1")
        f = open("test/data/1234-5678.raw", "rb")
        for block in iter(lambda: f.read(8000), ""):
            time.sleep(0.25)
            self.decoder_pipeline.process_data(block)

        self.decoder_pipeline.end_request()


        while not self.finished:
            time.sleep(1)
        self.assertEqual(["üks", "kaks", "kolm", "neli", "<#s>", "viis", "kuus", "seitse", "kaheksa", "<#s>"], self.words)

    def testWav(self):
        self.decoder_pipeline.init_request("testWav", "")
        f = open("test/data/lause2.wav", "rb")
        for block in iter(lambda: f.read(16000*2*2/4), ""):
            time.sleep(0.25)
            self.decoder_pipeline.process_data(block)

        self.decoder_pipeline.end_request()


        while not self.finished:
            time.sleep(1)
        self.assertEqual("see on teine lause <#s>".split(), self.words)

    def testOgg(self):
        self.decoder_pipeline.init_request("testOgg", "")
        f = open("test/data/test_2lauset.ogg", "rb")
        for block in iter(lambda: f.read(86*1024/8/4), ""):
            time.sleep(0.25)
            self.decoder_pipeline.process_data(block)

        self.decoder_pipeline.end_request()


        while not self.finished:
            time.sleep(1)
        self.assertEqual("see on esimene lause <#s> see on teine lause <#s>".split(), self.words)



    def __testDecoder(self):
        finished = [False]




        def do_shit():
            decoder_pipeline.init_request("test0", "audio/x-raw, layout=(string)interleaved, rate=(int)16000, format=(string)S16LE, channels=(int)1")
            f = open("test/data/1234-5678.raw", "rb")
            for block in iter(lambda: f.read(8000), ""):
                time.sleep(0.25)
                decoder_pipeline.process_data(block)
            
            decoder_pipeline.end_request()
    
        do_shit()
    
        while not finished[0]:
            time.sleep(1)
        self.assertEqual(["üks", "kaks", "kolm", "neli", "<#s>", "viis", "kuus", "seitse", "kaheksa", "<#s>"], words)
        
        words = []
        
        finished[0] = False    
        do_shit()
        while not finished[0]:
            time.sleep(1)
            
        self.assertItemsEqual(["see", "on", "teine", "lause", "<#s>"], words, "Recognition result")
        
        # Now test cancelation of a long submitted file
        words = []        
        decoder_pipeline.init_request("test0", "audio/x-raw, layout=(string)interleaved, rate=(int)16000, format=(string)S16LE, channels=(int)1")
        f = open("test/data/etteytlus.raw", "rb")
        decoder_pipeline.process_data(f.read())
        time.sleep(3)
        decoder_pipeline.cancel()
        print "Pipeline cancelled"
        
        words = []
        finished[0] = False
        decoder_pipeline.init_request("test0", "audio/x-raw, layout=(string)interleaved, rate=(int)16000, format=(string)S16LE, channels=(int)1")
        # read and send everything
        f = open("test/data/lause2.raw", "rb")
        decoder_pipeline.process_data(f.read(10*16000))
        decoder_pipeline.end_request()
        while not finished[0]:
            time.sleep(1)            
        self.assertItemsEqual(["see", "on", "teine", "lause", "<#s>"], words, "Recognition result")
        
        #test cancelling without anything sent
        decoder_pipeline.init_request("test0", "audio/x-raw, layout=(string)interleaved, rate=(int)16000, format=(string)S16LE, channels=(int)1")
        decoder_pipeline.cancel()
        print "Pipeline cancelled"
        
        words = []
        finished[0] = False
        decoder_pipeline.init_request("test0", "audio/x-wav")
        # read and send everything
        f = open("test/data/lause2.wav", "rb")
        decoder_pipeline.process_data(f.read())
        decoder_pipeline.end_request()
        while not finished[0]:
            time.sleep(1)            
        self.assertItemsEqual(["see", "on", "teine", "lause", "<#s>"], words, "Recognition result")

        words = []
        finished[0] = False
        decoder_pipeline.init_request("test0", "audio/ogg")
        # read and send everything
        f = open("test/data/test_2lauset.ogg", "rb")
        decoder_pipeline.process_data(f.read(10*16000))

        decoder_pipeline.end_request()
        while not finished[0]:
            time.sleep(1)
        self.assertItemsEqual("see on esimene lause <#s> see on teine lause <#s>".split(), words, "Recognition result")


def main():
    unittest.main()

if __name__ == '__main__':
    main()