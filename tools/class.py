class MSWF(workFlow): 
	def processCurrentState(self,inputModifierText,inputModifierImage): 
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("MS",inputModifierText[0])] 
			self.isEnd = True
			return
