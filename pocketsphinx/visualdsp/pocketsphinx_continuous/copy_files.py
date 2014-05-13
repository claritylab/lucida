import shutil
import sys
import os
import os.path

if len(sys.argv) > 1:
    src = sys.argv[1]
else:
    src = "C:\Program Files\Analog Devices\VisualDSP 4.5"

if not os.path.exists("CoreA"):
    print "Please run this script inside the VisualDSP++ project directory"
    exit(1)

if not os.path.exists(src):
    print src+" does not exist. Please provide the correct path as a command line parameter."
    exit(1)


if not os.path.exists(src+"/Blackfin/"):
    print src+" does not have Blackfin subdirectory which hints that you have given a wrong directory."
    exit(1)

shutil.copy(src+"/Blackfin/lib/src/libio/_stdio.h","CoreA")
shutil.copy(src+"/Blackfin/lib/src/libio/_wordsize.h","CoreA")
shutil.copy(src+"/Blackfin/lib/src/libio/xscan.c","CoreA")
shutil.copy(src+"/Blackfin/lib/src/drivers/codec/adi_ad1836a_ii.c","CoreA")
shutil.copy(src+"/Blackfin/Examples/Common Code/ezkitutilities.c",".")
shutil.copy(src+"/Blackfin/Examples/Common Code/ezkitutilities.h",".")
os.chdir("CoreA")
os.system("patch < xscan.c.patch")
os.system("patch < adi_ad1836a_ii.c.patch")
os.chdir("..")
