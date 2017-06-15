class QAWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData(\"QA\",inputModifierText[0])] 
			self.isEnd = True
			return
