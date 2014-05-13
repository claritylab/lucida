#!/usr/bin/python
# -*- coding: Cp1252 -*-

import glob
import os
import string
import binascii 

def scanfiles( dir ):
    res = []
    g = glob.glob( dir+'/*' )
    
    print 'scanning: ' + dir
    
    for x in g:
        
        x = x.translate( string.maketrans('\\','/') )
        
        if os.path.isdir(x):
            res.extend(scanfiles(x))
        else:
            data = binascii.hexlify(file(x,"rb").read())
            res.append( "\t{\"/" + x +'\",'+ str(len( data )/2) +',' )
            res.append( '\n\t\t"' )
            cnt = 0
            for ch in data:
                if (cnt == 80):
                    cnt = 0
                    res.append('"\\\n\t\t"')
                if (cnt % 2 == 0):
                    res.append('\\x')
                    
                res.append( ch )
                cnt = cnt + 1
            res.append( '"' )
            res.append( '},\n' )
            
    return res 

cdata = []

if os.path.exists('model'):
    cdata.append( '#include "model.h"\n\n#pragma file_attr("prefersMem =external")\n'\
'#pragma file_attr("prefersMemNum =30")\n#pragma section("memoryfile")\n\n' )
    cdata.append( 'const struct memoryfile modelfiles[] = {\n' )
    cdata.extend( scanfiles('model') ) 
    cdata.append( '\t{"",0,""}\n};\n' )
else:
    print 'model directory does not exist!'

file("CoreA/model.c","w" ).writelines(cdata)

print 'model.c created\n'
