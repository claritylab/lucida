"""
pygn.py

pygn (pronounced "pigeon") is a simple Python client for the Gracenote Music 
Web API, which can retrieve Artist, Album and Track metadata with the most 
common options.

You will need a Gracenote Client ID to use this module. Please contact 
developers@gracenote.com to get one.
"""

from __future__ import print_function
import xml.etree.ElementTree, json

try:
    import urllib.request as urllib_request #for python 3
    import urllib.parse   as urllib_parse

except ImportError:
    import urllib2 as urllib_request # for python 2
    import urllib as urllib_parse # for python 2

# Set DEBUG to True if you want this module to print out the query and response XML
DEBUG = False

class gnmetadata(dict):
	"""
	This class is a dictionary containing metadata fields that are available 
	for the queried item.
	"""
	def __init__(self):
		# Basic Metadata
		self['track_artist_name'] = ''
		self['album_artist_name'] = ''
		self['album_title'] = ''
		self['album_year'] = ''
		self['track_title'] = ''
		self['track_number'] = ''

		# Descriptors
		self['genre'] = {}
		self['artist_origin'] = {}
		self['artist_era'] = {}
		self['artist_type'] = {}
		self['mood'] = {}
		self['tempo'] = {}

		# Related Content
		self['album_art_url'] = ''
		self['artist_image_url'] = ''
		self['artist_bio_url'] = ''
		self['review_url'] = ''

		# Gracenote IDs
		self['album_gnid'] = ''
		self['track_gnid'] = ''

		#Radio ID
		self['radio_id'] = ''

		#  External IDs: Special content rights in license required
		self['xid'] =''

def register(clientID):
	"""
	This function registers an application as a user of the Gracenote service
	
	It takes as a parameter a clientID string in the form of 
	"NNNNNNN-NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN" and returns a userID in a 
	similar format.
	
	As the quota of number of users (installed applications or devices) is 
	typically much lower than the number of queries, best practices are for a
	given installed application to call this only once, store the UserID in 
	persistent storage (e.g. filesystem), and then use these IDs for all 
	subsequent calls to the service.
	"""
	
	# Create XML request
	query = _gnquery()
	query.addQuery('REGISTER')
	query.addQueryClient(clientID)
	
	queryXML = query.toString()
	
	# POST query
	response = urllib_request.urlopen(_gnurl(clientID), queryXML)
	responseXML = response.read()
	
	# Parse response
	responseTree = xml.etree.ElementTree.fromstring(responseXML)
	
	responseElem = responseTree.find('RESPONSE')
	if responseElem.attrib['STATUS'] == 'OK':
		userElem = responseElem.find('USER')
		userID = userElem.text
	
	return userID
#*****************************************************************************************************************************************		
# Added by Fabian in order to cover the Rhythm API
# Returns a list of gnmetadata dictionaries 

def createRadio(clientID='', userID='', artist='', track='', mood='', era='', genre='', popularity ='', similarity = '', count='10'):
	"""
	Queries a Radio playlist
	"""
	if clientID=='' or userID=='':
		print('ClientID and UserID are required')
		return None

	if artist=='' and track=='' and mood=='' and era=='' and genre=='':
		print('Must query with at least one field (artist, track, genre, mood, era)')
		return None

	#Create XML request
	query = _gnquery()
	
	# Build the user header 
	query.addAuth(clientID, userID)
	
	query.addQuery('RADIO_CREATE')
	
	if artist!='' or track!='':
		query.addTextSeed(artist,track)

	
	if mood!='' or era!='' or genre!='':
		query.addAttributeSeed(mood,era,genre)
			
	
	query.addQueryOption('SELECT_EXTENDED', 'COVER,REVIEW,ARTIST_BIOGRAPHY,ARTIST_IMAGE,ARTIST_OET,MOOD,TEMPO,LINK')
	query.addQueryOption('SELECT_DETAIL', 'GENRE:3LEVEL,MOOD:2LEVEL,TEMPO:3LEVEL,ARTIST_ORIGIN:4LEVEL,ARTIST_ERA:2LEVEL,ARTIST_TYPE:2LEVEL')	

	if popularity!='':
		query.addQueryOption('FOCUS_POPULARITY', popularity)
	if similarity!='':
		query.addQueryOption('FOCUS_SIMILARITY', similarity)


	query.addQueryOption('RETURN_COUNT', count)



	queryXML = query.toString()

	if DEBUG:
		print('QUERY:')
		print(queryXML)

	# POST query
	response = urllib_request.urlopen(_gnurl(clientID), queryXML)
	responseXML = response.read()


	myPlaylist = []

	for x in range(1, int(count)):
		track = _parseRadioMetadata(responseXML,x)		
		myPlaylist.append(track)
	
	#print(responseXML)
	
	return myPlaylist

#*****************************************************************************************************************************************		
# Added by Fabian in order to cover the Rhythm API
# Returns a list of gnmetadata dictionaries 


def radioEvent(clientID='', userID='', radioID='', gnID='', event ='TRACK_PLAYED', count='10', popularity ='', similarity = ''):

	if clientID=='' or userID=='':
		print('ClientID and UserID are required')
		return None

	if radioID=='' or gnID=='':
		print('Event query must contain the radioID and gnID')
		return None

	#Create XML request
	query = _gnquery()
	
	# Build the user header 
	query.addAuth(clientID, userID)
	
	query.addQuery('RADIO_EVENT')

	query.addRadioID(radioID)

	query.addQueryEVENT(event, gnID)
	
	query.addQueryOption('RETURN_COUNT', count)

	if popularity!='':
		query.addQueryOption('FOCUS_POPULARITY', popularity)
	if similarity!='':
		query.addQueryOption('FOCUS_SIMILARITY', similarity)

	query.addQueryOption('SELECT_EXTENDED', 'COVER,REVIEW,ARTIST_BIOGRAPHY,ARTIST_IMAGE,ARTIST_OET,MOOD,TEMPO,LINK')
	query.addQueryOption('SELECT_DETAIL', 'GENRE:3LEVEL,MOOD:2LEVEL,TEMPO:3LEVEL,ARTIST_ORIGIN:4LEVEL,ARTIST_ERA:2LEVEL,ARTIST_TYPE:2LEVEL')

	query.addQueryOption('RETURN_SETTINGS', 'YES')
	
	queryXML = query.toString()
	if DEBUG:	
		print('QUERY:')
		print(queryXML)

	
	# POST query
	response = urllib_request.urlopen(_gnurl(clientID), queryXML)
	responseXML = response.read()


	myPlaylist = []

	for x in range(1, int(count)):
		track = _parseRadioMetadata(responseXML,x)		
		myPlaylist.append(track)
	
	print(responseXML)
	
	return myPlaylist
	

	
	

#***********************************************************************************************************************

def search(clientID='', userID='', artist='', album='', track='', toc=''):
	"""
	Queries the Gracenote service for a track, album, artist, or TOC
	
	TOC is a string of offsets in the format '150 20512 30837 50912 64107 78357 ...' 
	"""

	if clientID=='' or userID=='':
		print('ClientID and UserID are required')
		return None

	if artist=='' and album=='' and track=='' and toc=='':
		print('Must query with at least one field (artist, album, track, toc)')
		return None
	
	# Create XML request
	query = _gnquery()
	
	query.addAuth(clientID, userID)
	
	if (toc != ''):
		query.addQuery('ALBUM_TOC')
		query.addQueryMode('SINGLE_BEST_COVER')
		query.addQueryTOC(toc)
	else:
		query.addQuery('ALBUM_SEARCH')
		query.addQueryMode('SINGLE_BEST_COVER')
		query.addQueryTextField('ARTIST', artist)
		query.addQueryTextField('ALBUM_TITLE', album)
		query.addQueryTextField('TRACK_TITLE', track)
	query.addQueryOption('SELECT_EXTENDED', 'COVER,REVIEW,ARTIST_BIOGRAPHY,ARTIST_IMAGE,ARTIST_OET,MOOD,TEMPO')
	query.addQueryOption('SELECT_DETAIL', 'GENRE:3LEVEL,MOOD:2LEVEL,TEMPO:3LEVEL,ARTIST_ORIGIN:4LEVEL,ARTIST_ERA:2LEVEL,ARTIST_TYPE:2LEVEL')
	
	queryXML = query.toString()
	
	if DEBUG:
		print('------------')
		print('QUERY XML')
		print('------------')
		print(queryXML)
	
	# POST query
	response = urllib_request.urlopen(_gnurl(clientID), queryXML)
	responseXML = response.read()
	
	if DEBUG:
		print('------------')
		print('RESPONSE XML')
		print('------------')
		print(responseXML)

	# Create GNTrackMetadata object
	metadata = gnmetadata()
	
	# Parse response
	responseTree = xml.etree.ElementTree.fromstring(responseXML)
	responseElem = responseTree.find('RESPONSE')
	if responseElem.attrib['STATUS'] == 'OK':
		# Find Album element
		albumElem = responseElem.find('ALBUM')

		# Parse album metadata
		metadata['album_gnid'] = _getElemText(albumElem, 'GN_ID')
		metadata['album_artist_name'] = _getElemText(albumElem, 'ARTIST')
		metadata['album_title'] = _getElemText(albumElem, 'TITLE')
		metadata['album_year'] = _getElemText(albumElem, 'DATE')
		metadata['album_art_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'COVERART')
		metadata['genre'] = _getMultiElemText(albumElem, 'GENRE', 'ORD', 'ID')
		metadata['artist_image_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_IMAGE')
		metadata['artist_bio_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_BIOGRAPHY')
		metadata['review_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'REVIEW')
		
		# Look for OET
		artistOriginElem = albumElem.find('ARTIST_ORIGIN')
		if artistOriginElem is not None:
			metadata['artist_origin'] = _getMultiElemText(albumElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			metadata['artist_era'] = _getMultiElemText(albumElem, 'ARTIST_ERA', 'ORD', 'ID')
			metadata['artist_type'] = _getMultiElemText(albumElem, 'ARTIST_TYPE', 'ORD', 'ID')
		else:
			# Try to get OET again by fetching album by GNID
			metadata['artist_origin'], metadata['artist_era'], metadata['artist_type'] = _getOET(clientID, userID, metadata['album_gnid'])
			
		# Parse track metadata
		matchedTrackElem = albumElem.find('MATCHED_TRACK_NUM')
		if matchedTrackElem is not None:
			trackElem = albumElem.find('TRACK')
			
			metadata['track_number'] = _getElemText(trackElem, 'TRACK_NUM')
			metadata['track_gnid'] = _getElemText(trackElem, 'GN_ID')
			metadata['track_title'] = _getElemText(trackElem, 'TITLE')
			metadata['track_artist_name'] = _getElemText(trackElem, 'ARTIST')

			metadata['mood'] = _getMultiElemText(trackElem, 'MOOD', 'ORD', 'ID')
			metadata['tempo'] = _getMultiElemText(trackElem, 'TEMPO', 'ORD', 'ID')
				
			
			# If track-level GOET exists, overwrite metadata from album			
			if trackElem.find('GENRE') is not None:
				metadata['genre']	= _getMultiElemText(trackElem, 'GENRE', 'ORD', 'ID')
			if trackElem.find('ARTIST_ORIGIN') is not None:
				metadata['artist_origin'] = _getMultiElemText(trackElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			if trackElem.find('ARTIST_ERA') is not None:
				metadata['artist_era'] = _getMultiElemText(trackElem, 'ARTIST_ERA', 'ORD', 'ID')
			if trackElem.find('ARTIST_TYPE') is not None:
				metadata['artist_type'] = _getMultiElemText(trackElem, 'ARTIST_TYPE', 'ORD', 'ID')

		# Parse tracklist
		metadata['tracks'] = []
		for trackElem in albumElem.iter('TRACK'):
			trackdata = {}
			
			trackdata['track_number'] = _getElemText(trackElem, 'TRACK_NUM')
			trackdata['track_gnid'] = _getElemText(trackElem, 'GN_ID')
			trackdata['track_title'] = _getElemText(trackElem, 'TITLE')
			trackdata['track_artist_name'] = _getElemText(trackElem, 'ARTIST')

			trackdata['mood'] = _getMultiElemText(trackElem, 'MOOD', 'ORD', 'ID')
			trackdata['tempo'] = _getMultiElemText(trackElem, 'TEMPO', 'ORD', 'ID')
			
			# If track-level GOET exists, overwrite metadata from album			
			if trackElem.find('GENRE') is not None:
				trackdata['genre']	 = _getMultiElemText(trackElem, 'GENRE', 'ORD', 'ID')
			if trackElem.find('ARTIST_ORIGIN') is not None:
				trackdata['artist_origin'] = _getMultiElemText(trackElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			if trackElem.find('ARTIST_ERA') is not None:
				trackdata['artist_era'] = _getMultiElemText(trackElem, 'ARTIST_ERA', 'ORD', 'ID')
			if trackElem.find('ARTIST_TYPE') is not None:
				trackdata['artist_type'] = _getMultiElemText(trackElem, 'ARTIST_TYPE', 'ORD', 'ID')
			metadata['tracks'].append(trackdata)

		return metadata



def _parseRadioMetadata(responseXML, number):
	# Create GNTrackMetadata object
	metadata = gnmetadata()
	
	# Parse response
	responseTree = xml.etree.ElementTree.fromstring(responseXML)
	responseElem = responseTree.find('RESPONSE')
	if responseElem.attrib['STATUS'] == 'OK':
		#find the radio ID
		RadioElem = responseElem.find('RADIO')
		metadata['radio_id'] = _getElemText(RadioElem, 'ID')
		
		# Find Album the right album element
		albums = responseElem.findall('ALBUM')
		for albumElem in albums:
			if albumElem.attrib["ORD"] == str(number):
				# Parse album metadata
				metadata['album_gnid'] = _getElemText(albumElem, 'GN_ID')
				metadata['album_artist_name'] = _getElemText(albumElem, 'ARTIST')
				metadata['album_title'] = _getElemText(albumElem, 'TITLE')
				metadata['album_year'] = _getElemText(albumElem, 'DATE')
				metadata['album_art_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'COVERART')
				metadata['genre'] = _getMultiElemText(albumElem, 'GENRE', 'ORD', 'ID')
				metadata['artist_image_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_IMAGE')
				metadata['artist_bio_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_BIOGRAPHY')
				metadata['review_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'REVIEW')
	
				# Look for OET
				artistOriginElem = albumElem.find('ARTIST_ORIGIN')
				if artistOriginElem is not None:
					metadata['artist_origin'] = _getMultiElemText(albumElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
					metadata['artist_era'] = _getMultiElemText(albumElem, 'ARTIST_ERA', 'ORD', 'ID')
					metadata['artist_type'] = _getMultiElemText(albumElem, 'ARTIST_TYPE', 'ORD', 'ID')
				else:
					# Try to get OET again by fetching album by GNID
					metadata['artist_origin'], metadata['artist_era'], metadata['artist_type'] = _getOET(clientID, userID, metadata['album_gnid'])
		
				# Parse track metadata
				trackElem = albumElem.find('TRACK')
		
				metadata['track_number'] = _getElemText(trackElem, 'TRACK_NUM')
				metadata['track_gnid'] = _getElemText(trackElem, 'GN_ID')
				metadata['track_title'] = _getElemText(trackElem, 'TITLE')
				metadata['track_artist_name'] = _getElemText(trackElem, 'ARTIST')

				metadata['mood'] = _getMultiElemText(trackElem, 'MOOD', 'ORD', 'ID')
				metadata['tempo'] = _getMultiElemText(trackElem, 'TEMPO', 'ORD', 'ID')
		
				# If track-level GOET exists, overwrite metadata from album			
				if trackElem.find('GENRE') is not None:
					metadata['genre']	= _getMultiElemText(trackElem, 'GENRE', 'ORD', 'ID')
				if trackElem.find('ARTIST_ORIGIN') is not None:
					metadata['artist_origin'] = _getMultiElemText(trackElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
				if trackElem.find('ARTIST_ERA') is not None:
					metadata['artist_era'] = _getMultiElemText(trackElem, 'ARTIST_ERA', 'ORD', 'ID')
				if trackElem.find('ARTIST_TYPE') is not None:
					metadata['artist_type'] = _getMultiElemText(trackElem, 'ARTIST_TYPE', 'ORD', 'ID')
				if trackElem.find('XID') is not None: 
					metadata

				return metadata









def get_discography(clientID='', userID='', artist='', rangeStart=1, rangeEnd=10):
	"""
	Queries the Gracenote service for all albums containing an artist
	"""

	if clientID=='' or userID=='':
		print('ClientID and UserID are required')
		return None

	if artist=='':
		print('Must specify artist')
		return None

	# Create XML request
	query = _gnquery()

	query.addAuth(clientID, userID)
	query.addQuery('ALBUM_SEARCH')
	query.addQueryTextField('ARTIST', artist)
	query.addQueryOption('SELECT_EXTENDED', 'COVER,REVIEW,ARTIST_BIOGRAPHY,ARTIST_IMAGE,ARTIST_OET,MOOD,TEMPO')
	query.addQueryOption('SELECT_DETAIL', 'GENRE:3LEVEL,MOOD:2LEVEL,TEMPO:3LEVEL,ARTIST_ORIGIN:4LEVEL,ARTIST_ERA:2LEVEL,ARTIST_TYPE:2LEVEL')
	query.addQueryRange(rangeStart,rangeEnd)

	queryXML = query.toString()

	if DEBUG:
		print('------------')
		print('QUERY XML')
		print('------------')
		print(queryXML)

	# POST query
	response = urllib_request.urlopen(_gnurl(clientID), queryXML)
	responseXML = response.read()

	if DEBUG:
		print('------------')
		print('RESPONSE XML')
		print('------------')
		print(responseXML)

	# Create result array
	discography = []

	# Parse response
	responseTree = xml.etree.ElementTree.fromstring(responseXML)
	responseElem = responseTree.find('RESPONSE')
	if responseElem.attrib['STATUS'] == 'OK':
		# Find Album element
		albumElems = responseElem.findall('ALBUM')

	for albumElem in albumElems:

		metadata = gnmetadata()

		# Parse album metadata
		metadata['album_gnid'] = _getElemText(albumElem, 'GN_ID')
		metadata['album_artist_name'] = _getElemText(albumElem, 'ARTIST')

		metadata['album_title'] = _getElemText(albumElem, 'TITLE')
		metadata['album_year'] = _getElemText(albumElem, 'DATE')
		metadata['album_art_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'COVERART')
		metadata['genre'] = _getMultiElemText(albumElem, 'GENRE', 'ORD', 'ID')
		metadata['artist_image_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_IMAGE')
		metadata['artist_bio_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_BIOGRAPHY')
		metadata['review_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'REVIEW')

		# Look for OET
		artistOriginElem = albumElem.find('ARTIST_ORIGIN')
		if artistOriginElem is not None:
			metadata['artist_origin'] = _getMultiElemText(albumElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			metadata['artist_era'] = _getMultiElemText(albumElem, 'ARTIST_ERA', 'ORD', 'ID')
			metadata['artist_type'] = _getMultiElemText(albumElem, 'ARTIST_TYPE', 'ORD', 'ID')

		# Parse tracklist
		metadata['tracks'] = []
		for trackElem in albumElem.iter('TRACK'):
			trackdata = {}
			
			trackdata['track_number'] = _getElemText(trackElem, 'TRACK_NUM')
			trackdata['track_gnid'] = _getElemText(trackElem, 'GN_ID')
			trackdata['track_title'] = _getElemText(trackElem, 'TITLE')
			trackdata['track_artist_name'] = _getElemText(trackElem, 'ARTIST')

			trackdata['mood'] = _getMultiElemText(trackElem, 'MOOD', 'ORD', 'ID')
			trackdata['tempo'] = _getMultiElemText(trackElem, 'TEMPO', 'ORD', 'ID')
			
			# If track-level GOET exists, overwrite metadata from album			
			if trackElem.find('GENRE') is not None:
				trackdata['genre']	 = _getMultiElemText(trackElem, 'GENRE', 'ORD', 'ID')
			if trackElem.find('ARTIST_ORIGIN') is not None:
				trackdata['artist_origin'] = _getMultiElemText(trackElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			if trackElem.find('ARTIST_ERA') is not None:
				trackdata['artist_era'] = _getMultiElemText(trackElem, 'ARTIST_ERA', 'ORD', 'ID')
			if trackElem.find('ARTIST_TYPE') is not None:
				trackdata['artist_type'] = _getMultiElemText(trackElem, 'ARTIST_TYPE', 'ORD', 'ID')
			metadata['tracks'].append(trackdata)

		discography.append(metadata)

	return discography
	
def fetch(clientID='', userID='', GNID=''):
	"""
	Fetches a track or album by GN ID
	"""

	if clientID=='' or userID=='':
		print('ClientID and UserID are required')
		return None

	if GNID=='':
		print('GNID is required')
		return None
	
	# Create XML request
	query = _gnquery()

	query.addAuth(clientID, userID)
	query.addQuery('ALBUM_FETCH')
	query.addQueryGNID(GNID)
	query.addQueryOption('SELECT_EXTENDED', 'COVER,REVIEW,ARTIST_BIOGRAPHY,ARTIST_IMAGE,ARTIST_OET,MOOD,TEMPO')
	query.addQueryOption('SELECT_DETAIL', 'GENRE:3LEVEL,MOOD:2LEVEL,TEMPO:3LEVEL,ARTIST_ORIGIN:4LEVEL,ARTIST_ERA:2LEVEL,ARTIST_TYPE:2LEVEL')
	
	queryXML = query.toString()
	
	if DEBUG:
		print('------------')
		print('QUERY XML')
		print('------------')
		print(queryXML)
	
	# POST query
	response = urllib_request.urlopen(_gnurl(clientID), queryXML)
	responseXML = response.read()
	
	if DEBUG:
		print('------------')
		print('RESPONSE XML')
		print('------------')
		print(responseXML)

	# Create GNTrackMetadata object
	metadata = gnmetadata()
	
	# Parse response
	responseTree = xml.etree.ElementTree.fromstring(responseXML)
	responseElem = responseTree.find('RESPONSE')
	if responseElem.attrib['STATUS'] == 'OK':
	
		# Find Album element
		albumElem = responseElem.find('ALBUM')

		# Parse album metadata
		metadata['album_gnid'] = _getElemText(albumElem, 'GN_ID')
		metadata['album_artist_name'] = _getElemText(albumElem, 'ARTIST')
		metadata['album_title'] = _getElemText(albumElem, 'TITLE')
		metadata['album_year'] = _getElemText(albumElem, 'DATE')
		metadata['album_art_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'COVERART')
		metadata['genre'] = _getMultiElemText(albumElem, 'GENRE', 'ORD', 'ID')
		metadata['artist_image_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_IMAGE')
		metadata['artist_bio_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'ARTIST_BIOGRAPHY')
		metadata['review_url'] = _getElemText(albumElem, 'URL', 'TYPE', 'REVIEW')
	
		# Look for OET
		artistOriginElem = albumElem.find('ARTIST_ORIGIN')
		if artistOriginElem is not None:
			metadata['artist_origin'] = _getMultiElemText(albumElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			metadata['artist_era'] = _getMultiElemText(albumElem, 'ARTIST_ERA', 'ORD', 'ID')
			metadata['artist_type'] = _getMultiElemText(albumElem, 'ARTIST_TYPE', 'ORD', 'ID')
		else:
			# Try to get OET again by fetching album by GNID
			metadata['artist_origin'], metadata['artist_era'], metadata['artist_type'] = _getOET(clientID, userID, metadata['album_gnid'])
		
		# Parse track metadata
		matchedTrackElem = albumElem.find('MATCHED_TRACK_NUM')
		if matchedTrackElem is not None:
			trackElem = albumElem.find('TRACK')
		
			metadata['track_number'] = _getElemText(trackElem, 'TRACK_NUM')
			metadata['track_gnid'] = _getElemText(trackElem, 'GN_ID')
			metadata['track_title'] = _getElemText(trackElem, 'TITLE')
			metadata['track_artist_name'] = _getElemText(trackElem, 'ARTIST')

			metadata['mood'] = _getMultiElemText(trackElem, 'MOOD', 'ORD', 'ID')
			metadata['tempo'] = _getMultiElemText(trackElem, 'TEMPO', 'ORD', 'ID')
		
			# If track-level GOET exists, overwrite metadata from album			
			if trackElem.find('GENRE') is not None:
				metadata['genre']	= _getMultiElemText(trackElem, 'GENRE', 'ORD', 'ID')
			if trackElem.find('ARTIST_ORIGIN') is not None:
				metadata['artist_origin'] = _getMultiElemText(trackElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			if trackElem.find('ARTIST_ERA') is not None:
				metadata['artist_era'] = _getMultiElemText(trackElem, 'ARTIST_ERA', 'ORD', 'ID')
			if trackElem.find('ARTIST_TYPE') is not None:
				metadata['artist_type'] = _getMultiElemText(trackElem, 'ARTIST_TYPE', 'ORD', 'ID')

		# Parse tracklist
		metadata['tracks'] = []
		for trackElem in albumElem.iter('TRACK'):
			trackdata = {}
			
			trackdata['track_number'] = _getElemText(trackElem, 'TRACK_NUM')
			trackdata['track_gnid'] = _getElemText(trackElem, 'GN_ID')
			trackdata['track_title'] = _getElemText(trackElem, 'TITLE')
			trackdata['track_artist_name'] = _getElemText(trackElem, 'ARTIST')

			trackdata['mood'] = _getMultiElemText(trackElem, 'MOOD', 'ORD', 'ID')
			trackdata['tempo'] = _getMultiElemText(trackElem, 'TEMPO', 'ORD', 'ID')
			
			# If track-level GOET exists, overwrite metadata from album			
			if trackElem.find('GENRE') is not None:
				trackdata['genre']	 = _getMultiElemText(trackElem, 'GENRE', 'ORD', 'ID')
			if trackElem.find('ARTIST_ORIGIN') is not None:
				trackdata['artist_origin'] = _getMultiElemText(trackElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
			if trackElem.find('ARTIST_ERA') is not None:
				trackdata['artist_era'] = _getMultiElemText(trackElem, 'ARTIST_ERA', 'ORD', 'ID')
			if trackElem.find('ARTIST_TYPE') is not None:
				trackdata['artist_type'] = _getMultiElemText(trackElem, 'ARTIST_TYPE', 'ORD', 'ID')
			metadata['tracks'].append(trackdata)

		return metadata

def _gnurl(clientID):
	"""
	Helper function to form URL to Gracenote service
	"""
	clientIDprefix = clientID.split('-')[0]
	return 'https://c' + clientIDprefix + '.web.cddbp.net/webapi/xml/1.0/'
	
def _getOET(clientID, userID, GNID):
	"""
	Helper function to retrieve Origin, Era, and Artist Type by direct album 
	fetch
	"""
	# Create XML request
	query = _gnquery()
	
	query.addAuth(clientID, userID)
	query.addQuery('ALBUM_FETCH')
	query.addQueryGNID(GNID)
	query.addQueryOption('SELECT_EXTENDED', 'ARTIST_OET')
	query.addQueryOption('SELECT_DETAIL', 'ARTIST_ORIGIN:4LEVEL,ARTIST_ERA:2LEVEL,ARTIST_TYPE:2LEVEL')
	
	queryXML = query.toString()
	
	if DEBUG:
		print('------------')
		print('QUERY XML (from _getOET())')
		print('------------')
		print(queryXML)
	
	# POST query
	response = urllib_request.urlopen(_gnurl(clientID), queryXML)
	albumXML = response.read()
	
	if DEBUG:
		print('------------')
		print('RESPONSE XML (from _getOET())')
		print('------------')
		print(albumXML)
	
	# Parse XML
	responseTree = xml.etree.ElementTree.fromstring(albumXML)
	responseElem = responseTree.find('RESPONSE')
	if responseElem.attrib['STATUS'] == 'OK':
		albumElem = responseElem.find('ALBUM')
		artistOrigin = _getMultiElemText(albumElem, 'ARTIST_ORIGIN', 'ORD', 'ID')
		artistEra = _getMultiElemText(albumElem, 'ARTIST_ERA', 'ORD', 'ID')
		artistType = _getMultiElemText(albumElem, 'ARTIST_TYPE', 'ORD', 'ID')
	return artistOrigin, artistEra, artistType
	
class _gnquery:
	"""
	A utility class for creating and configuring an XML query for POST'ing to
	the Gracenote service
	"""

	def __init__(self):
		self.root = xml.etree.ElementTree.Element('QUERIES')
		
	def addAuth(self, clientID, userID):
		auth = xml.etree.ElementTree.SubElement(self.root, 'AUTH')
		client = xml.etree.ElementTree.SubElement(auth, 'CLIENT')
		user = xml.etree.ElementTree.SubElement(auth, 'USER')
	
		client.text = clientID
		user.text = userID
	
	def addQuery(self, cmd):
		query = xml.etree.ElementTree.SubElement(self.root, 'QUERY')
		query.attrib['CMD'] = cmd
	
	def addQueryMode(self, modeStr):
		query = self.root.find('QUERY')
		mode = xml.etree.ElementTree.SubElement(query, 'MODE')
		mode.text = modeStr

	def addQueryTextField(self, fieldName, value):
		query = self.root.find('QUERY')
		text = xml.etree.ElementTree.SubElement(query, 'TEXT')
		text.attrib['TYPE'] = fieldName
		text.text = value
	
	def addQueryOption(self, parameterName, value):
		query = self.root.find('QUERY')
		option = xml.etree.ElementTree.SubElement(query, 'OPTION')
		parameter = xml.etree.ElementTree.SubElement(option, 'PARAMETER')
		parameter.text = parameterName
		valueElem = xml.etree.ElementTree.SubElement(option, 'VALUE')
		valueElem.text = value
	
	def addQueryGNID(self, GNID):
		query = self.root.find('QUERY')
		GNIDElem = xml.etree.ElementTree.SubElement(query, 'GN_ID')
		GNIDElem.text = GNID
		
	def addQueryClient(self, clientID):
		query = self.root.find('QUERY')
		client = xml.etree.ElementTree.SubElement(query, 'CLIENT')
		client.text = clientID
		
	def addQueryRange(self, start, end):
		query = self.root.find('QUERY')
		queryRange = xml.etree.ElementTree.SubElement(query, 'RANGE')
		rangeStart = xml.etree.ElementTree.SubElement(queryRange, 'START')
		rangeStart.text = str(start)
		rangeEnd = xml.etree.ElementTree.SubElement(queryRange, 'END')
		rangeEnd.text = str(end)
	
	def addQueryTOC(self, toc):
		# TOC is a string of format '150 20512 30837 50912 64107 78357 ...' 
		query = self.root.find('QUERY')
		tocElem = xml.etree.ElementTree.SubElement(query, 'TOC')
		offsetElem = xml.etree.ElementTree.SubElement(tocElem, 'OFFSETS')
		offsetElem.text = toc
		
	def toString(self):
		return xml.etree.ElementTree.tostring(self.root)

	#Methods added by Fabian to reflect the Rhythm use case

	def addAttributeSeed(self, moodID, eraID, genreID):
		query = self.root.find('QUERY')
		seed = xml.etree.ElementTree.SubElement(query, 'SEED')
		seed.attrib['TYPE'] = "ATTRIBUTE"
		if genreID!='':
			genreElement = xml.etree.ElementTree.SubElement(seed, 'GENRE')
			genreElement.attrib['ID'] = genreID
		if moodID!='':		
			genreElement = xml.etree.ElementTree.SubElement(seed, 'MOOD')
			genreElement.attrib['ID'] = moodID
		if eraID!='':
			genreElement = xml.etree.ElementTree.SubElement(seed, 'ERA')
			genreElement.attrib['ID'] = eraID


	def addTextSeed(self, artist, track):
		query = self.root.find('QUERY')
		seed = xml.etree.ElementTree.SubElement(query, 'SEED')
		seed.attrib['TYPE'] = "TEXT"
		if artist!='':
			text = xml.etree.ElementTree.SubElement(seed, 'TEXT')
			text.attrib['TYPE'] = "ARTIST"
			text.text = artist
		if track!='':
			text = xml.etree.ElementTree.SubElement(seed, 'TEXT')
			text.attrib['TYPE'] = "TRACK"
			text.text = track
	
	def addQueryEVENT(self, eventType, gnID):
		query = self.root.find('QUERY')
		event = xml.etree.ElementTree.SubElement(query, 'EVENT')
		event.attrib['TYPE'] = eventType
		gnidTag = xml.etree.ElementTree.SubElement(event, 'GN_ID')
		gnidTag.text = gnID

	def addRadioID(self, radioID):
		query = self.root.find('QUERY')
		radio = xml.etree.ElementTree.SubElement(query, 'RADIO')
		myradioid = xml.etree.ElementTree.SubElement(radio, 'ID')
		myradioid.text = radioID


 
		 

def _getElemText(parentElem, elemName, elemAttribName=None, elemAttribValue=None):
	"""
	XML parsing helper function to find child element with a specific name, 
	and return the text value
	"""
	elems = parentElem.findall(elemName)
	for elem in elems:
		if elemAttribName is not None and elemAttribValue is not None:
			if elem.attrib[elemAttribName] == elemAttribValue:
				return urllib_parse.unquote(elem.text)
			else:
				continue
		else: # Just return the first one
			return urllib_parse.unquote(elem.text)
	return ''

def _getElemAttrib(parentElem, elemName, elemAttribName):
	"""
	XML parsing helper function to find child element with a specific name, 
	and return the value of a specified attribute
	"""
	elem = parentElem.find(elemName)
	if elem is not None:
		return elem.attrib[elemAttribName]

def _getMultiElemText(parentElem, elemName, topKey, bottomKey):
	"""
	XML parsing helper function to return a 2-level dict of multiple elements
	by a specified name, using topKey as the first key, and bottomKey as the second key
	"""
	elems = parentElem.findall(elemName)
	result = {} # 2-level dictionary of items, keyed by topKey and then bottomKey
	if elems is not None:
		for elem in elems:
			if topKey in elem.attrib:
				result[elem.attrib[topKey]] = {bottomKey:elem.attrib[bottomKey], 'TEXT':elem.text}
			else:
				result['0'] = {bottomKey:elem.attrib[bottomKey], 'TEXT':elem.text}
	return result
