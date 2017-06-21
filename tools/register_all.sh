#!/bin/bash

python ms_mongo.py add questionanswering QA localhost 8082 text text
python ms_mongo.py add imagematching IMM localhost 8083 image image
python ms_mongo.py add calendar CA localhost 8084 text none
python ms_mongo.py add imageclassification IMC localhost 8085 image none
python ms_mongo.py add facerecognition FACE localhost 8086 image none
python ms_mongo.py add digitrecognition DIG localhost 8087 image none
python ms_mongo.py add weather WE localhost 8088 text none
python ms_mongo.py add musicservice MS localhost 8089 text none

echo "class QAWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"QA\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add QAWF text\&text_image $PWD/../lucida/commandcenter/data/class_QAWF.txt
echo "class IMMWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"IMM\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add IMMWF image\&text_image $PWD/../lucida/commandcenter/data/class_IMMWF.txt
echo "class CAWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"CA\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add CAWF text $PWD/../lucida/commandcenter/data/class_CAWF.txt
echo "class IMCWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"IMC\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add IMCWF image\&text_image $PWD/../lucida/commandcenter/data/class_IMCWF.txt
echo "class FACEWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"FACE\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add FACEWF image\&text_image $PWD/../lucida/commandcenter/data/class_FACEWF.txt
echo "class DIGWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"DIG\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add DIGWF image\&text_image $PWD/../lucida/commandcenter/data/class_DIGWF.txt
echo "class WEWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"WE\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add WEWF text $PWD/../lucida/commandcenter/data/class_WEWF.txt
echo "class MSWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"MS\",inputModifierText[0])] 
			self.isEnd = True
			return" | python wf_mongo.py add MSWF text $PWD/../lucida/commandcenter/data/class_MSWF.txt

echo "All service installed successfully!"
