public class AQUAINTFactory {
	public AQUAINTPreprocessTemplate getAQUAINTType(String type) {
		if (args.length < 1) {
			MsgPrinter.printUsage("java AQUAINTFactory " + "directory");
			System.exit(1);
		}
		type = args[0];
		
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);		
			
		// add paragraph tags if missing
		MsgPrinter.printStatusMsg("Adding paragraph tags...");
		if (addParagraphTags())
			MsgPrinter.printStatusMsg("Paragraph tags added successfully.");
		else {
			MsgPrinter.printErrorMsg("Could not add paragraph tags.");
			System.exit(1);
		}		
		if(type == null) {
			return null;
		} else if(type.equalsIgnoreCase("AQUAINTPreprocess")) {
			return new AQUAINTPreprocess;
		} else if(type.equalsIgnoreCase("AQUAINT2Preprocess")) {
			return new AQUAINT2Preprocess;
		} else {
			return null;
		}
	}
} 