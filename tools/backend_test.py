from gui_backend import *

def main():
	db.add_service('questionanswering', 'QA', 'text', 'text')
	db.add_service('imagematching', 'IMM', 'image', 'image')
	db.add_service('calendar', 'CA', 'text', 'none')
	db.add_service('imageclassification', 'IMC', 'image', 'none')
	db.add_service('facerecognition', 'FACE', 'image', 'none')
	db.add_service('digitrecognition', 'DIG', 'image', 'none')
	db.add_service('weather', 'WE', 'text', 'none')
	db.add_service('musicservice', 'MS', 'text', 'none')
	
	QAWF_code = 'class QAWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	QAWF_code = QAWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("QA",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('QAWF', ['text', 'text_image'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_QAWF.txt', QAWF_code)
	IMMWF_code = 'class IMMWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	IMMWF_code = IMMWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("IMM",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('IMMWF', ['image', 'text_image'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_IMMWF.txt', IMMWF_code)
	CAWF_code = 'class CAWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	CAWF_code = CAWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("CA",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('CAWF', ['text'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_CAWF.txt', CAWF_code)
	IMCWF_code = 'class IMCWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	IMCWF_code = IMCWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("IMC",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('IMCWF', ['image', 'text_image'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_IMCWF.txt', IMCWF_code)
	FACEWF_code = 'class FACEWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	FACEWF_code = FACEWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("FACE",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('FACEWF', ['image', 'text_image'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_FACEWF.txt', FACEWF_code)
	DIGWF_code = 'class DIGWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	DIGWF_code = DIGWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("DIG",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('DIGWF', ['image', 'text_image'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_DIGWF.txt', DIGWF_code)
	WEWF_code = 'class WEWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	WEWF_code = WEWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("WE",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('WEWF', ['text'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_WEWF.txt', WEWF_code)
	MSWF_code = 'class MSWF(workFlow): \n\tdef processCurrentState(self,inputModifierText,inputModifierImage): \n\t\tif(self.currentState==0):'
	MSWF_code = MSWF_code + '\n\t\t\tself.batchedData = [serviceRequestData("MS",inputModifierText[0])] \n\t\t\tself.isEnd = True\n\t\t\treturn'
	db.add_workflow('MSWF', ['text'], '/home/zhexuan/Documents/lucida/lucida/commandcenter/data/class_MSWF.txt', MSWF_code)

if __name__ == '__main__':
	main()