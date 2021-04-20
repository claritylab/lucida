package info.ephyra.indexing;

public abstract class AQUAINTPreprocessorTemplate{
	/** Directory of the AQUAINT corpus */
	protected static String dir;

	// Common Methods
	public void preProcessing() {
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
	}
	

	@Override
	protected abstract static boolean addParagraphTags();
	
	
	
	
}