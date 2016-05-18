import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst

GObject.threads_init()
Gst.init(None)

appsrc = Gst.ElementFactory.make("appsrc", "appsrc")
filesink = Gst.ElementFactory.make("filesink", "filesink")
filesink.set_property("location", "test.dat")

pipeline = Gst.Pipeline()
pipeline.add(appsrc)
pipeline.add(filesink)
appsrc.link(filesink)
pipeline.set_state(Gst.State.PLAYING)

data = "1234" * 12
print "Using data: %s" % data

buf = Gst.Buffer.new_allocate(None, len(data), None)
buf.fill(0, data)
#for (i, c) in enumerate(data):
#    buf.memset(i, c, 1)
appsrc.emit("push-buffer", buf)

pipeline.send_event(Gst.Event.new_eos())

result = open("test.dat").read()

print "Result    : %s" % result