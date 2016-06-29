import sys
import os

import logging
logging.basicConfig(stream=sys.stderr)

current_dir =  os.path.abspath(os.path.dirname(__file__))
parent_dir = os.path.abspath(current_dir + "/../")

sys.path.insert(0, parent_dir)


with open(current_dir + "/envs.txt") as f:
   for line in f:
       os.environ[line.split("=")[0]] = line.split("=")[1][:-1]

from app import app as application
