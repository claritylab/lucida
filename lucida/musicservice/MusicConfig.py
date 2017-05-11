"""
Music API configuration
"""

from pygn import Pygn
from helper import port_dic

# Service port number
PORT = int(port_dic['ms_port'])

# Music API username and password
clientID = # TODO: Enter your Client ID from developer.gracenote.com here
userID = Pygn.register(clientID) # Get a User ID from pygn.register() - Only register once per end-user
