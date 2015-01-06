#!/usr/bin/env python

"""
setup.py file for SWIG example
"""

import sys
import os.path

def get_rootdir():
    return '/home/users/okazaki/local'
def get_includedir():
    return os.path.join(get_rootdir(), 'include')
def get_librarydir():
    return os.path.join(get_rootdir(), 'lib')

import os; os.environ['CC'] = 'g++'; os.environ['CXX'] = 'g++';
os.environ['CPP'] = 'g++'; os.environ['LDSHARED'] = 'g++'

from distutils.core import setup, Extension

crfsuite_module = Extension(
    '_crfsuite',
    sources = [
        'crfsuite.cpp',
        'export_wrap.cpp',
        ],
#    include_dirs=['../../include',],
    extra_link_args=['-shared'],
#    library_dirs=['../../lib/crf',],
    libraries=['crfsuite'],
#    extra_objects=['../../lib/crf/libcrfsuite.la'],
    language='c++',
    )

setup(
    name = '@PACKAGE@',
    version = '@VERSION@',
    author = 'Naoaki Okazaki',
    description = """CRFSuite Python module""",
    ext_modules = [crfsuite_module],
    py_modules = ["crfsuite"],
    )

