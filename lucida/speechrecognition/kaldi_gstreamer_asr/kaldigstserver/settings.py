'''
Created on Jun 7, 2013

@author: tanel
'''

from tornado.options import define

define("port", default=8888, help="run on the given port", type=int)
