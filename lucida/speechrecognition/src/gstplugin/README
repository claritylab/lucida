WHAT IT IS
----------

gst-plugin is a template for writing your own GStreamer plug-in.

The code is deliberately kept simple so that you quickly understand the basics
of how to set up autotools and your source tree.

This template demonstrates :
- what to do in autogen.sh
- how to setup configure.ac (your package name and version, GStreamer flags)
- how to setup your source dir 
- what to put in Makefile.am

More features and templates might get added later on.

HOW TO USE IT
-------------

To use it, either make a copy for yourself and rename the parts or use the
make_element script in tools. To create sources for "myfilter" based on the
"gsttransform" template run:

cd src;
../tools/make_element myfilter gsttransform

This will create gstmyfilter.c and gstmyfilter.h. Open them in an editor and
start editing. There are several occurances of the string "template", update
those with real values. The plugin will be called 'myfilter' and it will have
one element called 'myfilter' too. Also look for "FIXME:" markers that point you
to places where you need to edit the code.

You still need to adjust the Makefile.am.

