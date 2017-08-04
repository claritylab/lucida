import pygame, sys, os
from time import sleep
from pygame.locals import *
pygame.init()
import math
from gui_backend_cloud import db
import random
import base64
import dill
from cStringIO import StringIO
import pygame, sys, eztext
from textrect import *


# make sure no file named .hiddentempfile is under this folder
def input_impl():
	os.system('gedit .hiddentempfile')
	file = open('.hiddentempfile')
	code = ""
	for line in file:
		code = code + line
	os.system('rm .hiddentempfile')
	print(code)
	return code


#used for modifying code for state/branch
def writeFileForUseMod(inputWrite):
	text_file = open(".hiddentempfile", "w")
	text_file.write(inputWrite)
	text_file.close()



def getUserInputText():
    # initialize pygame
    pygame.init()
    # create the screen
    screen = pygame.display.set_mode((1280,240))
    # fill the screen w/ white
    screen.fill((255,255,255))
    # here is the magic: making the text input
    # create an input with a max length of 45,
    # and a red color and a prompt saying 'type here: '
    txtbx = eztext.Input(maxlength=245, color=(255,0,0), prompt='type: ')
    # create the pygame clock
    clock = pygame.time.Clock()
    # main loop!

    while 1:
        # make sure the program is running at 30 fps
        clock.tick(30)

        # events for txtbx
        events = pygame.event.get()
        # process other events
        for event in events:
            # close it x button si pressed
            if event.type == QUIT: 
				return txtbx.value
				

        # clear the screen
        screen.fill((255,255,255))
        # update txtbx
        txtbx.update(events)
        # blit txtbx on the sceen
        txtbx.draw(screen)
        # refresh the display
        pygame.display.flip()
        



class imageObject(object):
	def __init__(self,name,xPos,yPos,xWidth,yWidth,imageThing,textInBox=" "):
		#These are in 2D vector space.
		self.xWidth = xWidth
		self.yWidth = yWidth
		self.xPos = xPos
		self.yPos = yPos
		self.text = textInBox
		self.imageThing = imageThing
		self.drawLevel = 0
		self.name = name
	def drawImage(self,screen,screenWidth,screenHeight):
		widthInPixels = int(screenWidth*self.xWidth)
		heightInPixels = int(screenHeight*self.yWidth)
		self.imageThing = pygame.transform.scale(self.imageThing, (int(screenWidth*self.xWidth),int(screenHeight*self.yWidth)))
		screen.blit(self.imageThing, (self.xPos*screenWidth, self.yPos*screenHeight))
		
		my_rect = pygame.Rect((self.xPos*screenWidth, self.yPos*screenHeight, widthInPixels , heightInPixels))
    
		rendered_text = render_textrect(self.text, myfont, my_rect, (0,0,0), (48, 255, 48), 1)
		if rendered_text:
			screen.blit(rendered_text, my_rect.topleft)
		
	def setDrawLevel(self,valueSet):
		self.drawLevel = valueSet
	def isPointOnImage(self,xVectorSpace,yVectorSpace):
		if((xVectorSpace>self.xPos) and (xVectorSpace<(self.xPos+self.xWidth)) and (yVectorSpace>self.yPos) and (yVectorSpace<(self.yPos+self.yWidth))):
			return "YUP"
		return "NOPE"


def drawArrow(screen,point0x,point0y,point1x,point1y):

	pos1=point0x,point0y
	pos2=point1x,point1y

	pygame.draw.line(screen, (0,0,0), pos1, pos2)

	arrow=pygame.Surface((50,50))
	arrow.fill((255,255,255))
	pygame.draw.line(arrow, (0,0,0), (0,0), (25,25))
	pygame.draw.line(arrow, (0,0,0), (0,50), (25,25))
	arrow.set_colorkey((255,255,255))

	angle=math.atan2(-(pos1[1]-pos2[1]), pos1[0]-pos2[0])
	angle=math.degrees(angle)

	def drawAng(angle, pos):
		nar=pygame.transform.rotate(arrow,angle)
		nrect=nar.get_rect(center=pos)
		screen.blit(nar, nrect)

	angle+=180
	drawAng(angle, pos2)

	
	
	
	

	
class lineObject(object):
	def __init__(self,name,xPos,yPos,xPosEnd,yPosEnd):
		#These are in 2D vector space.
		self.xPos = xPos
		self.yPos = yPos
		self.xPosEnd = xPosEnd
		self.yPosEnd = yPosEnd
		self.drawLevel = 0
		self.name = name
	def drawImage(self,screen,screenWidth,screenHeight):
		startX = self.xPos*screenWidth
		startY = self.yPos*screenHeight
		endX = self.xPosEnd*screenWidth
		endY = self.yPosEnd*screenHeight
		pygame.draw.line(windowScreen.screen,0,(startX,startY),(endX,endY),1)
		
		
		
		drawArrow(screen,startX,startY,endX,endY)		
	
	def setDrawLevel(self,valueSet):
		self.drawLevel = valueSet
	def isPointOnImage(self,xVectorSpace,yVectorSpace):
		return "NOPE"

class windowInformation(object):
	def __init__(self,x,y):
		self.width = x
		self.height = y
		self.screen = pygame.display.set_mode((x, y))
		self.listOfObjectsNew = dict();
	
	def refreshWindow(self):
		self.screen = pygame.display.set_mode((self.width, self.height))
		
		# Go through every image object and draw to screen
	def blitScreen(self):
		print("=======BLITTING SCREEN=====")
		self.screen.fill(0)
		bgIMGtmp = bgIMG;
		bgIMGtmp = pygame.transform.scale(bgIMGtmp, (self.width,self.height))
		self.screen.blit(bgIMGtmp, (0,0))
		drawLevels = -1
		#Some images are supposed to draw ontop of otehrs
		while drawLevels!=10:
			drawLevels+=1
			for key in self.listOfObjectsNew:
				if(self.listOfObjectsNew[key].drawLevel==drawLevels):
					self.listOfObjectsNew[key].drawImage(self.screen,self.width,self.height)
		pygame.display.flip()
			
			
		#Add an image object onto the screen
	def addObject(self,objectName,objectThing):
		self.listOfObjectsNew[objectName]= objectThing;
		
	def removeObject(self,objectName):
		del self.listOfObjectsNew[objectName]
		
		
	def getClickOnName(self,xClick,yClick):
			xClickVectorSpace = float(xClick)/self.width
			yClickVectorSpace = float(yClick)/self.height
			
		
			
			#When checking where click, must go through the top to bottom of the image stack
			drawLevels = 10
			while drawLevels!=-1:
				
				for key in self.listOfObjectsNew:
					#print(drawLevels,key,self.listOfObjectsNew[key].drawLevel)
					if(self.listOfObjectsNew[key].drawLevel==drawLevels):
						status = self.listOfObjectsNew[key].isPointOnImage(xClickVectorSpace,yClickVectorSpace)
						#print("NODE CHECK", key,xClickVectorSpace,yClickVectorSpace,status,self.listOfObjectsNew[key].drawLevel,drawLevels)
						if(status=="YUP"):
							 return key
				drawLevels-=1
			

			return "NOPE"
				
		
		
		
		
		
		
		
		
		
		
		
# Load the image types
yellowBG = pygame.image.load(os.path.join("images/yellowBG.png"))
grayBG = pygame.image.load(os.path.join("images/grayBG.png"))
dead = pygame.image.load(os.path.join("images/dead.png"))
		
nodeActiveIMG = pygame.image.load(os.path.join("images/nodeActive.png"))
nodeIMG = pygame.image.load(os.path.join("images/node.png"))
serviceListButtonIMG = pygame.image.load(os.path.join("images/serviceListButton.png"))
bgIMG = pygame.image.load(os.path.join("images/bg.png"))





def getNodeImage(node):
	if(node.active==1):
		return nodeActiveIMG
	return nodeIMG



textOffsetX = 0.025
textOffsetY = 0.025

class lucidaGUI(object):
	def __init__(self):
		self.dirList = []
		self.WFXOffset = 0.0
		self.WFYOffset = -0.0
		self.currentLevelName = ""
		
			
	#Clicking on level, find what is being clicked on.
	def clickLevel(self,xClick,yClick):
		clickOnName = windowScreen.getClickOnName(xClick,yClick)
		if(clickOnName!="NOPE"):
			self.level(clickOnName,1)
		
		
		
	def refreshCurrent(self):
			windowScreen.refreshWindow()
			upDirName = directoriesNames.pop()
			lucida.level(upDirName,0)
		
	def getXYPositionEntry(self,ID):
		x = ID%5
		ID /= 5
		y = ID
		return [x*0.1,y*0.1]
		
	def goUpDir(self, time):
		countArr = len(directoriesNames)
		
		if countArr>1:
			upDirName = directoriesNames.pop()
			for count in range(time):
				upDirName = directoriesNames.pop()
			self.level(upDirName,0)

			
	def updateObjectPosition(self,name,xPos,yPos):
		for objectThing in self.dirList:
			if(objectThing.name==name):
				objectThing.xPos = xPos
				objectThing.yPos = yPos
				objectThing.drawImage(windowScreen.screen,windowScreen.width,windowScreen.height)
				
	def updateObjectPosition2(self,name,xPos,yPos):
		for objectThing in self.dirList:
			if(objectThing.name==name):
				objectThing.xPosEnd = xPos
				objectThing.yPosEnd = yPos
				objectThing.drawImage(windowScreen.screen,windowScreen.width,windowScreen.height)
					
				
				
	def updateObjectImage(self,name,imageThing):
		for objectThing in self.dirList:
			if(objectThing.name==name):
				objectThing.imageThing = imageThing
				objectThing.drawImage(windowScreen.screen,windowScreen.width,windowScreen.height)
		
	def addToLevel(self,xPos,yPos,xWidth,yWidth,drawLevel,imageObjectName,image,text=" "):
			imageObjectInst = imageObject(imageObjectName, xPos,yPos,xWidth,yWidth,image,text)
			imageObjectInst.setDrawLevel(drawLevel)
			self.dirList.append(imageObjectInst)
	


	def addToLevelLine(self,xPos,yPos,drawLevel,imageObjectName,xPosEnd,yPosEnd):
			textObjectInst = lineObject(imageObjectName,xPos,yPos,xPosEnd,yPosEnd)
			textObjectInst.setDrawLevel(drawLevel)
			self.dirList.append(textObjectInst)	
		
		
	# Removes the current display
	def undisplayCurrentLevel(self):

		for objectThing in self.dirList:
			windowScreen.removeObject(objectThing.name)
		self.dirList = []
	# Adds the current (And possibly new) display
	def displayCurrentLevel(self):
		print("======++DISPLAY CURRENT++======")
		for objectThing in self.dirList:
			windowScreen.addObject(objectThing.name,objectThing)
		windowScreen.blitScreen();
		
	def getXYNode(self,node):
		XY = []
		XY.append(node.x-self.WFXOffset)
		XY.append(node.y-self.WFYOffset)
		return XY
		
			
	#Level function draws the level display and the level click functions
	def level(self,levelName,directClick):
		print("start")
		print(directoriesNames)
		#print("Hit level", levelName)
		if(directClick==1):
			self.currentLevelName = levelName
			
			
		didHit = 0
		goUpDirtime = 0
		self.undisplayCurrentLevel();
		### Front: CMD/MS buttons
		if(levelName=="root"):
			didHit = 1
			self.addToLevel(0.25,0.4,0.5,0.1,0,"choosemode",yellowBG,"Please choose mode!")
			self.addToLevel(0.25,0.5,0.25,0.1,0,"root_local",grayBG,"Local")
			self.addToLevel(0.5,0.5,0.25,0.1,0,"root_docker",grayBG,"Docker")

		if(levelName=="root_local"):
			didHit = 1
			self.addToLevel(0.25,0.4,0.5,0.1,0,"wfList",yellowBG,"Workflows")
			self.addToLevel(0.25,0.5,0.5,0.1,0,"msList",yellowBG,"Microservices")
			self.addToLevel(0.25,0.6,0.5,0.1,0,"bbList",yellowBG,"Blackboxes")
			
			
		# List all of the workflow types
		if(levelName=="wfList"):
			didHit = 1
			countServices = 0;
			
			self.addToLevel(0.25,0.25,0.25,0.10,0,"wfListHeader:",yellowBG,"Workflows:")
			self.addToLevel(0.50,0.25,0.25,0.10,0,"newWF:",yellowBG,"New")
			for WF in workflowList:
				XY = self.getXYPositionEntry(countServices)
				self.addToLevel(0.25+XY[0],0.35+XY[1],0.10,0.10,0,"wfThing:"+str(countServices),grayBG,WF.name)
				countServices+= 1
				
		
		## bbList has not been implemented in the GUI yet
		if(levelName=="bbList"):
			didHit = 1
			countServices = 0;
			
			self.addToLevel(0.25,0.25,0.50,0.10,0,"bbListHeader:",yellowBG,"Microservices:")
			for MS in microServiceList:
				XY = self.getXYPositionEntry(countServices)
				self.addToLevel(0.25+XY[0],0.35+XY[1],0.10,0.10,0,"bbListThing"+str(countServices),grayBG,MS.name)
				countServices+= 1

		if "bbListThing" in levelName:
			didHit = 1
			getMSID = int(filter(str.isdigit, levelName))
			msName = microServiceList[getMSID].name
			self.addToLevel(0.25,0.25,0.50,0.10,0,"MicroservicesNameThing:"+str(getMSID),yellowBG,"MS: " + msName)
			countServices = 0
			for server in microServiceList[getMSID].serverList:
				XY = self.getXYPositionEntry(countServices)
				self.addToLevel(0.25+XY[0],0.35+XY[1],0.10,0.10,0,"bbinstance" + str(getMSID) + "bbinstance"+str(countServices),grayBG,server.name)
				countServices+= 1

		if "bbinstance" in levelName:
			didHit = 1
			print("BELOW THIS LINE")
			idList = levelName.split("bbinstance")
			msID = int(idList[1])
			boxID = int(idList[2])
			print(levelName.split("bbinstance"))
			box = microServiceList[msID].serverList[boxID]
			msName = microServiceList[msID].name
			boxName = box.name
			
			self.addToLevel(0.25,0.25,0.25,0.10,0,"MS:" + msName,yellowBG,"MS: " + msName)
			self.addToLevel(0.50,0.25,0.25,0.10,0,"BoxNameBB:",yellowBG,"Box: " + boxName)
			self.addToLevel(0.25,0.35,0.50,0.10,0,"StartServer:" +str(msID) + "StartServer:" + str(boxID),yellowBG,"Start!")
			
		if "StartServer:" in levelName:
			idList = levelName.split("StartServer:")
			msID = int(idList[1])
			boxID = int(idList[2])
			box = microServiceList[msID].serverList[boxID]
			status = db.start_server(microServiceList[msID].dbData['_id'], box.dbData['id'])
			self.refreshCurrent()		

				
		#List a specific workflow
		if "wfThing:" in levelName:
			didHit = 1
			getWFID = int(filter(str.isdigit, levelName))
			wfName = workflowList[getWFID].name
			self.addToLevel(0.25,0.25,0.35,0.10,0,"WorkflowNameThing:"+str(getWFID),yellowBG,"WF: " + wfName)
			self.addToLevel(0.60,0.25,0.25,0.10,0,"wfListThing"+str(getWFID),yellowBG,"State Graph")
			

			inputTypes = ', '.join(workflowList[getWFID].dbData['input'])

			self.addToLevel(0.25,0.35,0.60,0.10,0,"wfType:"+str(getWFID),yellowBG,"Type:"  + inputTypes)
			self.addToLevel(0.25,0.45,0.60,0.25,0,"classPath:"+str(getWFID),yellowBG,"ClassPath:" + workflowList[getWFID].dbData['classifier'])
			self.addToLevel(0.25,0.70,0.60,0.10,0,"DeleteWorkflow"+str(getWFID),dead,"Delete "+wfName)
				
				
				

		# This is the actual state graph
		if "wfListThing" in levelName:
			didHit = 1
			countServices = 0;
			getWFID = int(filter(str.isdigit, levelName))
			WF = workflowList[getWFID]
			print(WF.nodeList)
			
			self.addToLevel(0.0,0.0,1.0,1.0,0,"wfBG:",bgIMG) # This gives the background of the state graph

			countServices = 0;
			for node in WF.nodeList:
				XY = self.getXYNode(node)
				
				
				self.addToLevel(XY[0],XY[1],0.10,0.10,1,"node:"+str(countServices),getNodeImage(node),node.name)
				
				
				#Display the links from this node to another
				countLinks = 0
				for link in node.linkList:
					toNode = WF.nodeList[link.toNode]
					XY2 = self.getXYNode(toNode)
					lineThickness = 0.01;
					self.addToLevelLine(XY[0],XY[1],2,"IAmLine:"+str(countServices)+"IAmLine:"+str(countLinks),XY2[0],XY2[1])

					
					countLinks += 1
				
				countServices += 1



			
			
		#List microservices
		if(levelName=="msList"):
			didHit = 1
			countServices = 0;
			
			self.addToLevel(0.25,0.25,0.25,0.10,0,"msListHeader:",yellowBG,"Microservices:")
			self.addToLevel(0.50,0.25,0.25,0.10,0,"newMicroService",yellowBG,"New")
			for MS in microServiceList:
				XY = self.getXYPositionEntry(countServices)
				self.addToLevel(0.25+XY[0],0.35+XY[1],0.10,0.10,0,"msListThing"+str(countServices),grayBG,MS.name)
				countServices+= 1
				
		#Display a specific microservice
		if "msListThing" in levelName:
			didHit = 1
			getMSID = int(filter(str.isdigit, levelName))
			msName = microServiceList[getMSID].name
			self.addToLevel(0.25,0.25,0.25,0.10,0,"MicroservicesNameThing:"+str(getMSID),yellowBG,"MS: " + msName)
			self.addToLevel(0.50,0.25,0.25,0.10,0,"newServer"+str(getMSID),yellowBG,"New")
			self.addToLevel(0.25,0.35,0.50,0.05,0,"LearnType:"+str(getMSID),yellowBG,"LearnType: " + microServiceList[getMSID].dbData['learn'])
			self.addToLevel(0.25,0.40,0.50,0.05,0,"InputType:"+str(getMSID),yellowBG,"InputType: " + microServiceList[getMSID].dbData['input'])
			self.addToLevel(0.75,0.75,0.20,0.10,0,"DeleteService"+str(getMSID),dead,"Delete "+msName)

			countServices = 0
			for server in microServiceList[getMSID].serverList:
				XY = self.getXYPositionEntry(countServices)
				self.addToLevel(0.25+XY[0],0.45+XY[1],0.10,0.10,0,"serverBB" + str(getMSID) + "serverBB"+str(countServices),grayBG,server.name)
				countServices+= 1
		
		# An instance of the service
		if "serverBB" in levelName:
			didHit = 1
			print("BELOW THIS LINE")
			idList = levelName.split("serverBB")
			msID = int(idList[1])
			boxID = int(idList[2])
			print(levelName.split("serverBB"))
			box = microServiceList[msID].serverList[boxID]
			msName = microServiceList[msID].name
			boxName = box.name
			
			self.addToLevel(0.25,0.25,0.25,0.10,0,"MS:" + msName,yellowBG,"MS: " + msName)
			self.addToLevel(0.50,0.25,0.25,0.10,0,"BoxNameSet:" +str(msID) + "BoxNameSet:" + str(boxID),yellowBG,"Box: " + boxName)
			self.addToLevel(0.25,0.35,0.50,0.05,0,"IP:PORT"+str(msID)+"IP:PORT"+str(boxID),yellowBG,"IP:PORT:" + str(box.IP) + ":" + str(box.port))
			self.addToLevel(0.25,0.40,0.50,0.35,0,"LOCATION"+str(msID)+"LOCATION"+str(boxID),yellowBG,"Location:"+str(box.location))
			self.addToLevel(0.25,0.75,0.50,0.10,0,"DeleteInstance"+str(msID)+"DeleteInstance"+str(boxID),dead,"Delete "+boxName)
		
		if "LOCATION" in levelName:
			idList = levelName.split("LOCATION")
			msID = int(idList[1])
			boxID = int(idList[2])
			box = microServiceList[msID].serverList[boxID]
			returnText = getUserInputText()
			status = db.update_instance(microServiceList[msID].dbData['_id'], box.dbData['id'], "location", returnText)
			generateMicroServiceList()
			self.refreshCurrent()

		if "DeleteInstance" in levelName:
			didHit = 1
			goUpDirtime=2
			idList = levelName.split("DeleteInstance")
			msID = int(idList[1])
			boxID = int(idList[2])
			box = microServiceList[msID].serverList[boxID]
			status = db.delete_instance(microServiceList[msID].dbData['_id'], box.dbData['id'])
			generateMicroServiceList()

		if "DeleteService" in levelName:
			didHit = 1
			goUpDirtime=2
			getMSID = int(filter(str.isdigit, levelName))
			status = db.delete_service(microServiceList[getMSID].dbData['_id'])
			generateMicroServiceList()

		if "DeleteWorkflow" in levelName:
			didHit = 1
			goUpDirtime=2
			getWFID = int(filter(str.isdigit, levelName))
			status = db.delete_workflow(workflowList[getWFID].dbData['_id'])
			generateWorkflowList()

			
		#Change the name of a microservice
		if "MicroservicesNameThing:" in levelName:
			idList = levelName.split("MicroservicesNameThing:")
			msID = int(idList[1])
			returnText = getUserInputText()
			status = db.update_service( microServiceList[msID].dbData['_id'], "name", returnText)
			status = db.update_service( microServiceList[msID].dbData['_id'], "acronym", returnText)
			generateMicroServiceList()
			self.refreshCurrent()
			
		# Change the name of a classifier path
		if "classPath:" in levelName:
			idList = levelName.split("classPath:")
			wfID = int(idList[1])
			returnText = getUserInputText()
			status = db.update_workflow( workflowList[wfID].dbData['_id'], "classifier", returnText)
			generateWorkflowList()
			self.refreshCurrent()
			
			
		#Change the name of a workflow
		if "WorkflowNameThing:" in levelName:
			idList = levelName.split("WorkflowNameThing:")
			wfID = int(idList[1])
			returnText = getUserInputText()
			status = db.update_workflow( workflowList[wfID].dbData['_id'], "name", returnText)
			generateWorkflowList()
			workflowData = compileWorkflow(wfID)
			db.update_workflow(workflowList[wfID].dbData['_id'], "code",workflowData);
			self.refreshCurrent()
			
			
			
		#Change the IP:port of a microservice
		if "IP:PORT" in levelName:
			returnText = getUserInputText()
			idList = levelName.split("IP:PORT")
			msID = int(idList[1])
			boxID = int(idList[2])
			box = microServiceList[msID].serverList[boxID]
			ipportinfo = returnText.split(":")
			box.IP = ipportinfo[0]
			box.port = int(ipportinfo[1])
			print(microServiceList[msID].dbData['_id'] , "SPACE" , box.dbData['id'] , "SPACE" + box.IP)
			status = db.update_instance(microServiceList[msID].dbData['_id'], box.dbData['id'], "host", box.IP)
			status = db.update_instance(microServiceList[msID].dbData['_id'], box.dbData['id'], "port", box.port)

			generateMicroServiceList()
			self.refreshCurrent()
			
		#Name an instance
		if "BoxNameSet:" in levelName:
			returnText = getUserInputText()
			idList = levelName.split("BoxNameSet:")
			msID = int(idList[1])
			boxID = int(idList[2])
			box = microServiceList[msID].serverList[boxID]
			status = db.update_instance(microServiceList[msID].dbData['_id'], box.dbData['id'], "name", returnText)
			generateMicroServiceList()
			self.refreshCurrent()
			
			
			#Toggles a service between learn types
		if "LearnType:" in levelName:
			didHit=1
			goUpDirtime=1
			getMSID = int(filter(str.isdigit, levelName))
			if(microServiceList[getMSID].dbData['learn']=='text'):
				status = db.update_service( microServiceList[getMSID].dbData['_id'], "learn", "image")
			if(microServiceList[getMSID].dbData['learn']=='image'):
				status = db.update_service( microServiceList[getMSID].dbData['_id'], "learn", "none")	
			if(microServiceList[getMSID].dbData['learn']=='none'):
				status = db.update_service( microServiceList[getMSID].dbData['_id'], "learn", "text")
			generateMicroServiceList()

		if "InputType:" in levelName:
			didHit=1
			goUpDirtime=1
			getMSID = int(filter(str.isdigit, levelName))
			if(microServiceList[getMSID].dbData['input']=='text'):
				status = db.update_service( microServiceList[getMSID].dbData['_id'], "input", "image")
			if(microServiceList[getMSID].dbData['input']=='image'):
				status = db.update_service( microServiceList[getMSID].dbData['_id'], "input", "text")	
			generateMicroServiceList() 
	
			#Toggles a workflow between input types
		if "wfType:" in levelName:
			didHit=1
			goUpDirtime=1
			getWFID = int(filter(str.isdigit, levelName))
			
			if(set(workflowList[getWFID].dbData['input'])==set(['text', 'image', 'text_image'])):
				status = db.update_workflow( workflowList[getWFID].dbData['_id'], "input", ['image'])			
			elif(workflowList[getWFID].dbData['input']==['image']):
				status = db.update_workflow( workflowList[getWFID].dbData['_id'], "input", ['text'])
			elif(workflowList[getWFID].dbData['input']==['text']):
				status = db.update_workflow( workflowList[getWFID].dbData['_id'], "input", ['text_image'])
			elif(workflowList[getWFID].dbData['input']==['text_image']):
				status = db.update_workflow( workflowList[getWFID].dbData['_id'], "input", ['text','text_image'])
			elif(set(workflowList[getWFID].dbData['input'])==set(['text', 'text_image'])):
				status = db.update_workflow( workflowList[getWFID].dbData['_id'], "input", ['image','text_image'])
			elif(set(workflowList[getWFID].dbData['input'])==set(['image', 'text_image'])):
				status = db.update_workflow( workflowList[getWFID].dbData['_id'], "input", ['text','image'])
			elif(set(workflowList[getWFID].dbData['input'])==set(['text', 'image'])):
				status = db.update_workflow( workflowList[getWFID].dbData['_id'], "input", ['text','image','text_image'])
				

				
			generateWorkflowList()
	
	
	
	
		#New blackbox service registry
		if "newServer" in levelName:
			didHit=1
			goUpDirtime=1
			microserviceNameOfServerID = int(levelName.split("newServer",1)[1])
			print(microServiceList[microserviceNameOfServerID].dbData['_id'])
			db.add_instance(microServiceList[microserviceNameOfServerID].dbData['_id'])
			generateMicroServiceList()

		#A new workflow
		if "newWF:" in levelName:
			didHit=1
			goUpDirtime=1
			status,idWF = db.add_workflow()
			
			db.update_workflow(idWF, "name", "WFTaco");
			db.update_workflow(idWF, "input", ["text"]);
			db.update_workflow(idWF, "classifier", "/home/masonhill/lucidas/lucidaapi/lucida-api/lucida/commandcenter/data/class_QAWF.txt");
			
			generateWorkflowList()

		#Add a new micro service
		if(levelName=="newMicroService"):
			didHit=1
			goUpDirtime=1
			returnVal = 1;
			newAppend = 0;
			
			while(returnVal!=0):
				returnVal, serviceID = db.add_service()
				newAppend+= 1
				
			generateMicroServiceList()

 

		if(didHit==1):
			countArr = len(directoriesNames)
			if countArr>0:
				previousDirName = directoriesNames.pop()
				directoriesNames.append(previousDirName)
				if(previousDirName!=levelName):
					directoriesNames.append(levelName)
			if(countArr==0):
				directoriesNames.append(levelName)

			print("mid")
			print(directoriesNames)
			#directoriesNames.append(levelName)
			self.displayCurrentLevel();
		
		if goUpDirtime>=1:
			self.goUpDir(goUpDirtime)

			
			#HIt nothing, so need to refresh previous state
		if(didHit==0):
			upDirName = directoriesNames.pop()
			lucida.level(upDirName,0)
		
		if(not "wfListThing" in directoriesNames[-1]):
			currentActiveNode = -1

		print("end")
		print(directoriesNames)






class server(object):
	def __init__(self,name):
		self.name = name
		self.IP = "127.0.0.1"
		self.port = 0
		self.location = ""
		


class microService(object):
	def __init__(self,name):
		self.name = name
		self.serverList = []
		
		
	def addServer(self,name,ip="",port=0,location=""):
		serverThing = server(name)
		self.serverList.append(serverThing)
		serverThing.IP = ip
		serverThing.port = port
		serverThing.location = location
		
class linkObj(object):
	def __init__(self,name,toNode):
		self.name = name
		self.toNode = toNode
		self.code = "if(condArgs['pug']=='25 years'):\r\n\tself.pause = False\r\n\treturn True;"
		

class node(object):
	def __init__(self,name):
		self.name = name
		self.linkList = [] # Contains a list of node links
		self.x = 0.0;
		self.y = 0.0
		self.active = 0
		self.code = "exeMS('pug',\"QA\",\"How old is Johann?\")\nEXE_BATCHED #THIS STATEMENT MUST EXIST AFTER ALL exeMS STATEMENTS. No exeMS is executed UNTIL AFTER THIS STATEMENT\ncondArgs['pug'] = batchedDataReturn['pug']\nself.ret = condArgs['pug']"
		
	def addLink(self,linkName,toNode):
		problemHere = linkObj(linkName,toNode)
		self.linkList.append(problemHere)
		
		
class workflow(object):
	def __init__(self,name):
		self.name = name
		self.nodeList = []
		self.xOffset = 0.0
		self.yOffset = 0.0
		
	def addNode(self,node):
		self.nodeList.append(node)
		
#A workflow needs to convert from stategraph to actual python code
def compileWorkflow(workflowID):

	workflowCompiled = ""

	# Each link needs its own function



	workflowName = workflowList[workflowID].name;

	
	workflowCompiled += "\nclass "+workflowName+"(workFlow):"

	
	
	nodeCount = 0
	for node in workflowList[workflowID].nodeList:

		linkCount = 0
		for linkObj in node.linkList:
			
			
			workflowCompiled += "\n\tdef branchCheck"+workflowName+str(nodeCount)+"_"+str(linkCount)+"(self,condArgs,passArgs):";
			
	

						
			stri = StringIO(linkObj.code)
			while True:
				nl = stri.readline()
				if nl == '': break
				workflowCompiled += "\n\t\t"+nl

			
			
			
			linkCount +=1
	
		nodeCount +=1
		
	
	workflowCompiled += "\n\r\n\tdef processCurrentState(self,batchingBit,batchedDataReturn,passArgs,inputModifierText,inputModifierImage):"
	
	workflowCompiled += "\n\t\tcondArgs = dict()"
	nodeCount = 0
	
	for node in workflowList[workflowID].nodeList:
		workflowCompiled += "\n\t\tif(self.currentState==" + str(nodeCount) +"):"

		workflowCompiled += "\n\t\t\tif(batchingBit==1): self.batchedData = []"

					
		stri = StringIO(node.code)
		while True:
			nl = stri.readline()
			appendExeMSString = ""
			if nl == '': break
			#Need to check the line if it contains a batched request entry. This is [varname] = exeMS( format
			currentCount = 0;
			for c in nl:
				
				'''
				'''		
				
					
			nl = nl.replace("exeMS(", "if(batchingBit==1): self.batchedData = appendServiceRequest(self.batchedData,")
			nl = nl.replace("EXE_BATCHED", "if(batchingBit==1): return 2")
			workflowCompiled += "\n\t\t\t"+nl

		workflowCompiled += "\n\t\t\tif(1==0): QUANTUM_PHYSICS()"
		
		linkCount = 0
		for linkObj in node.linkList:
			
			workflowCompiled += "\n\t\t\telif(self.branchCheck"+workflowName+str(nodeCount)+"_"+str(linkCount)+"(condArgs,passArgs)):"
			workflowCompiled += "\n\t\t\t\tself.currentState = " + str(linkObj.toNode)

			
			linkCount +=1
		workflowCompiled += "\n\t\t\telse: self.isEnd = True"
	
		workflowCompiled += "\n\t\t\treturn self.pause, self.ret"
		nodeCount +=1
	
	return workflowCompiled





directoriesNames = []
workflowList = []
microServiceList = []




# OBJ->DILL->DISK WRITE->DISK READ->BINARY BLOB->HEX64
#This converts any object to base64.
def objToBase64(workflowThing):
	#OBJ->DILL->DISK WRITE
	with open('wf.tmp', 'wb') as f:
		dill.dump(workflowThing, f)

	#DISK READ->BINARY BLOB->HEX64
	file = open('wf.tmp', 'rb')
	file_content = file.read()
	base64_two = base64.b64encode(file_content)
	return base64_two
	


#This converts base64 to obj
def base64ToObj(hexData):
	#UN-HEX64->BINARY BLOB->DISK WRITE
	writeData = hexData.decode('base64');

	text_file = open('wf2.tmp', 'wb')
	text_file.write(writeData)
	text_file.close()
	#DISK READ->DILL->OBJ
	with open('wf2.tmp', 'rb') as f:
		test = dill.load( f)
		return test


def generateMicroServiceList():
	
	global microServiceList
	microServiceList = []
	serviceList = db.get_services();
	count = 0
	for service in serviceList:
		#Create the microservice
		if(service['name']==''): service['name'] = "NULL"
		microServiceList.append(microService(service['name']))
		microServiceList[count].dbData = service
		#And add a list of the valid microservice instances
		countInst = 0
		for instance in service['instance']:
			if(instance['name']==""): instance['name'] = "NULL"
			microServiceList[count].addServer(instance['name'],instance['host'],instance['port'],instance['location'])
			microServiceList[count].serverList[countInst].dbData = instance
			countInst+=1
		count+= 1

	

def generateWorkflowList():


	
	print("^^^^^^^^^^^^^^^^^^^WORKFLOW^^^^^^^^^^^^^^^^^^^^^")
	global workflowList
	workflowList = []
	workflows = db.get_workflows();
	count = 0
	for workflowI in workflows:
		#Create the microservice
		if( workflowI['name']==''):  workflowI['name'] = "NULL"
		workflowList.append(workflow( workflowI['name']))
		workflowList[count].dbData =  workflowI
		if( workflowI['stategraph']!=""):
			workflowList[count].nodeList = base64ToObj( workflowI['stategraph'])
		count+= 1

	


generateMicroServiceList()
generateWorkflowList();







windowScreen = windowInformation(512,512)

myfont = pygame.font.SysFont("arial", 14)
# render text
valueInc = 0


lucida = lucidaGUI()
lucida.level("root",0)

debounceKey = []




#Create the debounce key stack
loop255 = 0;
while loop255!=255:
	debounceKey.append(0)
	loop255+=1



debounceMouse = []
debounceMouse.append(0)
debounceMouse.append(0)
debounceMouse.append(0)

oldMousePosition = []
oldMousePosition.append(-1)
oldMousePosition.append(-1)
currentActiveNode = -1


draggingNode = 0
draggingWF = -1
draggingNodeID = -1
draggingX = 0.0
draggingY = 0.0

draggingScreen = 0
draggingScreenWF = -1
draggingScreenX = 0.0
draggingScreenY = 0.0


debounceHm = 0;

while True:
	
	

	sleep(0.03)
	p = pygame.mouse.get_pos();
	keysPressed = pygame.key.get_pressed()
	
	#Check if backspace is hit
	if(keysPressed[8]==1 and debounceKey[8]==0):
		if(currentActiveNode!=-1):
			nodeData = WF.nodeList[currentActiveNode]
			currentActiveNode = -1
			lucida.updateObjectImage(lucida.currentLevelName,nodeIMG)
			nodeData.active = 0
		lucida.goUpDir(1)
		 

		 
		 
	mousePress = pygame.mouse.get_pressed()
	
	#Get old mouse position (For moving screen around)
	if(mousePress[0]==1 and debounceMouse[0]==1):
		if(oldMousePosition[0]==-1):
			oldMousePosition[0] = p[0]
			oldMousePosition[1] = p[1]

	
	#This creates a new node in the WF
	if(keysPressed[110]==1 and debounceKey[110]==0 and "wfListThing" in directoriesNames[-1]):
		getWFID = int(filter(str.isdigit, directoriesNames[-1]))
		WF = workflowList[getWFID]
		WF.addNode(node("some state"))
		lucida.level(directoriesNames[-1],0);
	
	
	
	#This checks if we are moving a node around.
	if(mousePress[0]==1 and debounceMouse[0]==1 and "wfListThing" in directoriesNames[-1] and "node:" in lucida.currentLevelName):
		draggingNode = 1
		idList = lucida.currentLevelName.split("node:")
		getWFID = int(filter(str.isdigit, directoriesNames[-1]))
		WF = workflowList[getWFID]
		nodeID = int(idList[1])
		draggingWF = getWFID
		draggingNodeID = nodeID

		#Should be able to update now.
		nodeData = WF.nodeList[nodeID]
		xVector = ((float(p[0]-oldMousePosition[0]))/windowScreen.width)
		yVector = ((float(p[1]-oldMousePosition[1]))/windowScreen.height)

		draggingX = xVector+nodeData.x
		draggingY = yVector+nodeData.y
			
		xPlaceNew = xVector+nodeData.x-lucida.WFXOffset
		yPlaceNew = yVector+nodeData.y-lucida.WFYOffset
		lucida.updateObjectPosition(lucida.currentLevelName,xPlaceNew,yPlaceNew)
		lucida.updateObjectPosition("wfListText:"+str(nodeID),xPlaceNew+textOffsetX,yPlaceNew+textOffsetY)
		
		linkCount = 0
		#This updates the lines at node. 
		for link in nodeData.linkList:
			lucida.updateObjectPosition("IAmLine:" + str(nodeID) + "IAmLine:" + str(linkCount),xPlaceNew,yPlaceNew)
			linkCount += 1

		#But if for instance S0->S1, S0 is updating the line as it owns the node. But if S1 moves, it is not being updated as S1 does not own the node
		#So must go through every single node, and find all toNodes and see if match
		
		fromNodeID = 0
		for nodeCheck in WF.nodeList:
			
			linkCount = 0;
			for link in nodeCheck.linkList:
				toNodeID = link.toNode
				if(toNodeID==nodeID): # This means eg. S0->S1. We are moving S1. This means S0 has a node to S1, so the line from S0 needs to update.
					#So update S0 to repoint to S1.
					lucida.updateObjectPosition2("IAmLine:" + str(fromNodeID) + "IAmLine:" + str(linkCount),xPlaceNew,yPlaceNew)
				linkCount+=1
			fromNodeID+=1
		

		windowScreen.blitScreen()


	## Update the state graph in DB.
	if(draggingNode==1 and mousePress[0] == 0):
		draggingNode=0
		WF = workflowList[draggingWF]
		nodeData = WF.nodeList[draggingNodeID]
		nodeData.x =draggingX
		nodeData.y = draggingY
		base64content = objToBase64(WF.nodeList)
		db.update_workflow(workflowList[draggingWF].dbData['_id'], "stategraph", base64content);

	
		
	# With initial mouse point, now drag around
	if(mousePress[0]==1 and debounceMouse[0]==1 and "wfListThing" in directoriesNames[-1] and "wfBG:" in lucida.currentLevelName):
		draggingScreen = 1
		differencePoint = [(float(p[0]-oldMousePosition[0]))/windowScreen.width,(float((p[1]-oldMousePosition[1]))/windowScreen.height)]
		getWFID = int(filter(str.isdigit, directoriesNames[-1]))
		WF = workflowList[getWFID]

		countServices = 0;
		for nodeData in WF.nodeList:
			draggingScreenX = -differencePoint[0]
			draggingScreenY = -differencePoint[1]
			
			xPlaceNew = nodeData.x+differencePoint[0]-lucida.WFXOffset
			yPlaceNew = nodeData.y+differencePoint[1]-lucida.WFYOffset
			lucida.updateObjectPosition("node:"+str(countServices),xPlaceNew,yPlaceNew)
			lucida.updateObjectPosition("wfListText:"+str(countServices),xPlaceNew+textOffsetX,yPlaceNew+textOffsetY)

			linkCount = 0
			for link in nodeData.linkList:

				lucida.updateObjectPosition("IAmLine:" + str(countServices) + "IAmLine:" + str(linkCount),xPlaceNew,yPlaceNew)
				linkCount += 1
			
			# S0->S1. So S0 gets hit. It has link to S1. So S0 updates its from. What is its to? Reference S1.
		
			linkCount = 0;
			for link in nodeData.linkList:
				toNodeID = link.toNode
				nodeDataTo = WF.nodeList[toNodeID];
				xPlaceNew2 = nodeDataTo.x+differencePoint[0]-lucida.WFXOffset
				yPlaceNew2 = nodeDataTo.y+differencePoint[1]-lucida.WFYOffset
			
				lucida.updateObjectPosition2("IAmLine:" + str(countServices) + "IAmLine:" + str(linkCount),xPlaceNew2,yPlaceNew2)
				linkCount += 1

			countServices += 1
			
			
			
			
			
		windowScreen.blitScreen()
	
	
	if(draggingScreen==1 and mousePress[0] == 0):
		draggingScreen=0
		WF = workflowList[draggingScreenWF]
		lucida.WFXOffset += draggingScreenX
		lucida.WFYOffset += draggingScreenY
	
	if(mousePress[0]==1 and debounceMouse[0]==0):
		lucida.clickLevel(p[0],p[1])
	
	
	#Modify a workflow logic
	if(currentActiveNode!=-1 and "wfListThing" in directoriesNames[-1] and keysPressed[109]==1 and debounceKey[109]==0):
		getWFID = int(filter(str.isdigit, directoriesNames[-1]))
		WF = workflowList[getWFID]
		nodeThing = WF.nodeList[currentActiveNode]
		writeFileForUseMod(nodeThing.code)
		nodeThing.code = input_impl() #User code
		workflowData = compileWorkflow(getWFID)
		db.update_workflow(WF.dbData['_id'], "code",workflowData);



	######## Lets check if I pressed on a line.
	if(mousePress[0]==1 and currentActiveNode==-1 and debounceMouse[0]==0 and "wfListThing" in directoriesNames[-1] and "node:" not in lucida.currentLevelName):
		getWFID = int(filter(str.isdigit, directoriesNames[-1]))
		WF = workflowList[getWFID]
		#So go through every single line and see if within certain distance
		# And to do that I must go through every single node.
		countServices = 0;
		floatP = []
		floatP.append(float(p[0])/windowScreen.width)
		floatP.append(float(p[1])/windowScreen.height)
		print(floatP)
		for nodeData in WF.nodeList:
			xStart = nodeData.x
			yStart = nodeData.y;
			
			countServices+=1;
			linkCount = 0;
			for link in nodeData.linkList:
				toNodeID = link.toNode
				nodeDataTo = WF.nodeList[toNodeID];
				xEnd = nodeDataTo.x
				yEnd = nodeDataTo.y

				#no division by 0
				if(xStart==xEnd):
					 xStart = 1;
				m = (yStart-yEnd)/(xStart-xEnd);
				b = yStart-m*xStart
				d = abs((m*floatP[0])+(floatP[1]*-1) + b)/math.sqrt(m*m+1);
				# This line is being pressed.
				if(d<0.01  and keysPressed[109]==1 ):
					writeFileForUseMod(link.code)
					link.code = input_impl()
					workflowData = compileWorkflow(getWFID)
					db.update_workflow(WF.dbData['_id'], "code",workflowData);
				linkCount+=1
		pass
	
	
	# Unselect node, select node, and make link
	if(mousePress[0]==1 and debounceMouse[0]==0 and "wfListThing" in directoriesNames[-1] and "node:" in lucida.currentLevelName):
		idList = lucida.currentLevelName.split("node:")
		getWFID = int(filter(str.isdigit, directoriesNames[-1]))
		WF = workflowList[getWFID]
		nodeID = int(idList[1])
		nodeData = WF.nodeList[nodeID]
		
		if(currentActiveNode==nodeID): # Click again to disable?
			if(keysPressed[108]==1):
				nodeDataOriginal = WF.nodeList[currentActiveNode]
				isLinked = 0
				#Check for if already linked. If no link, link, if link, unlink.
				for link in nodeDataOriginal.linkList:
					if(link.toNode==nodeID):
						isLinked = 1
					
					#Link them	
				if(isLinked==0):
					nodeDataOriginal.addLink("linkName",nodeID);
					lucida.level(directoriesNames[-1],1);
					pass
				elif (isLinked==1): #TODO: Unlink
					pass
				
				base64content = objToBase64(WF.nodeList)
				db.update_workflow(workflowList[getWFID].dbData['_id'], "stategraph", base64content);
			else:
				print("Unselect node")
				currentActiveNode = -1
				lucida.updateObjectImage(lucida.currentLevelName,nodeIMG)
				nodeData.active = 0
		elif(currentActiveNode==-1): # No previous selected, so select
			print("Select Node:", nodeID)
			currentActiveNode = nodeID
			lucida.updateObjectImage(lucida.currentLevelName,nodeActiveIMG)
			nodeData.active = 1
		else: # There is a selected, and it is not the selected. So make link?
			print("Make link", currentActiveNode, nodeID)
			#Check if "l" is pressed when attempting to link two nodes.
			nodeDataOriginal = WF.nodeList[currentActiveNode]
			
			#If "l" is pressed, AND not already linked, make link.
			if(keysPressed[108]==1):
				isLinked = 0
				#Check for if already linked. If no link, link, if link, unlink.
				for link in nodeDataOriginal.linkList:
					if(link.toNode==nodeID):
						isLinked = 1
					
					#Link them	
				if(isLinked==0):
					nodeDataOriginal.addLink("linkName",nodeID);
					lucida.level(directoriesNames[-1],1);
					pass
				elif (isLinked==1): #TODO: Unlink
					pass
				
				base64content = objToBase64(WF.nodeList)
				db.update_workflow(workflowList[getWFID].dbData['_id'], "stategraph", base64content);

				
				
				
			
			#If "l" is not pressed, this means selecting a new node.
			if(keysPressed[108]==0):
				
				lucida.updateObjectImage("node:"+str(currentActiveNode),nodeIMG)
				nodeDataOriginal.active = 0
				currentActiveNode = nodeID
				lucida.updateObjectImage(lucida.currentLevelName,nodeActiveIMG)
				nodeData.active = 1

	
	
	
	
	
	
	
	
	
	if(mousePress[0]==0):
		oldMousePosition = []
		oldMousePosition.append(-1)
		oldMousePosition.append(-1)
		
		
		
		
	#Debounce the key stack
	loop255 = 0;
	while loop255!=255:
		debounceKey[loop255] = keysPressed[loop255]
		loop255+=1

	#Debounce the mouse
	debounceMouse[0] = mousePress[0]
	
	#Makes the program terminate if the x on the window is pressed
	for event in pygame.event.get():
		if event.type == pygame.QUIT:
			crashTheProgram()
		pass

pygame.quit()