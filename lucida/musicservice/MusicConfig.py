"""
Music API configuration
"""

from pygn import Pygn

# Music API username and password
clientID = '173703779-EA19716F8D2A73DF7ECAF522D5050BF3' # TODO: Enter your Client ID from developer.gracenote.com here
userID = Pygn.register(clientID) # Get a User ID from pygn.register() - Only register once per end-user
