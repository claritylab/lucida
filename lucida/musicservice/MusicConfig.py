# Music API configuration
from pygn import Pygn

# Service port number
PORT = 8089

# Music API username and password
clientID =  # TODO: Enter your Client ID from developer.gracenote.com here
userID = Pygn.register(clientID) # Get a User ID from pygn.register() - Only register once per end-user
