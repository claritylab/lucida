package lucida.handler;

// Open Ephyra packages
import info.ephyra.OpenEphyra;
import info.ephyra.search.Result;
import info.ephyra.io.MsgPrinter;

// Java packages
import java.util.List;
import java.util.ArrayList;

// Interface definition
import lucida.thrift.*;

/** Implementation of the question-answer interface defined
 * in the question-answer thrift file. A client request to any
 * method defined in the thrift file is handled by the
 * corresponding method here.
 */
public class QAServiceHandler implements LucidaService.Iface {
	/** An object that lets the question-answer wrapper use
	 * the end-to-end OpenEphyra framework.
	 */
	private OpenEphyra oe;
	private String defaultAnswer;
	
	/** Constructs the handler and initializes its OpenEphyra
	 * object.
	 */
	public QAServiceHandler() {
		String dir = "";

		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);

		oe = new OpenEphyra(dir);
		defaultAnswer = "Factoid not found in knowledge base.";
	}
	
	/**
	 * Creates a new OpenEphyra service.
	 * @param LUCID ID of Lucida user
	 * @param spec spec
	 */
    public void create(String LUCID, QuerySpec spec) {
    	MsgPrinter.printStatusMsg("@@@@@ Create; User: " + LUCID);
    	User.addUser(LUCID);
    }

	/**
	 * Adds knowledge to OpenEphyra.
	 * @param LUCID ID of Lucida user
	 * @param knowledge knowledge
	 */
    public void learn(String LUCID, QuerySpec knowledge) {
    	MsgPrinter.printStatusMsg("@@@@@ Learn; User: " + LUCID);
    	KnowledgeBase kb = User.getUserKB(LUCID);
    	kb.addKnowledge(knowledge);
    	MsgPrinter.printStatusMsg(kb.toString());
    }

    /**
     * Extracts a query string and returns the answer from OpenEphyra.
	 * @param LUCID ID of Lucida user
	 * @param query query
     */
    public String infer(String LUCID, QuerySpec query) {
    	MsgPrinter.printStatusMsg("@@@@@ Infer; User: " + LUCID);
    	KnowledgeBase kb = User.getUserKB(LUCID);
    	kb.commitKnowledge();
    	// Set INDRI_INDEX.
    	System.setProperty("INDRI_INDEX", kb.getIndriIndex());
    	if (query.content.isEmpty() || query.content.get(0).data.isEmpty()) {
    		throw new IllegalArgumentException();
    	}
    	// Only look for the first item in content and data.
    	// The rest part of query is ignored.
    	return askFactoidThrift(query.content.get(0).data.get(0));
    }

	/** Forwards the client's question to the OpenEphyra object's askFactoid
	 * method and collects the response.
	 * @param question eg. "what is the speed of light?"
	 */
	private String askFactoidThrift(String question) {
		MsgPrinter.printStatusMsg("askFactoidThrift(): Arg = " + question);
		
		Result result = oe.askFactoid(question);
		String answer = defaultAnswer;
		if (result != null) {
			answer = result.getAnswer();
		}

		return answer;
	}

//	/** Forwards the client's question to the OpenEphyra object's askList
//	 * method and collects the response.
//	 * @param question eg. "name the US Presidents"
//	 */
//	public List<String> askListThrift(String question) {
//		float relThresh = 0.5f; //user may change this value
//		
//		MsgPrinter.printStatusMsg("askListThrift(): Arg = " + question);
//
//		Result[] results = oe.askList(question, relThresh);
//		List<String> answersList = new ArrayList<String>();
//		if (results.length > 0) {
//			// add all answers to answersList
//			for (Result r : results) {
//				answersList.add(r.getAnswer());
//			}
//		} else {
//			answersList.add(defaultAnswer);
//		}
//		return answersList;
//	}
}

