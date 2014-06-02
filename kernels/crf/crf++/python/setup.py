#!/usr/bin/env python

from distutils.core import setup,Extension,os
import string

setup(name = "mecab-python",
      py_modules=["CRFPP"],
      ext_modules = [Extension("_CRFPP",
                               ["CRFPP_wrap.cxx",],
                               libraries=["crfpp", "pthread"])
                     ])
