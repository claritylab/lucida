package lucida.handler;

// Open Ephyra packages
import info.ephyra.OpenEphyra;
import info.ephyra.search.Result;
import info.ephyra.io.MsgPrinter;

// Java packages
import java.util.List;
import java.util.ArrayList;

import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;

// Interface definition
import lucida.thrift.*;

/** Implementation of the question-answer interface defined
 * in the question-answer thrift file. A client request to any
 * method defined in the thrift file is handled by the
 * corresponding method here.
 */
public class QAServiceHandler {
	private static void print(String s) {
		synchronized (System.out) {
			System.out.println(s);
		}
	}

	public static class SyncQAServiceHandler implements LucidaService.Iface {
		/** An object that lets the question-answer wrapper use
		 * the end-to-end OpenEphyra framework.
		 */
		private OpenEphyra oe;
		private String defaultAnswer;
		
		/** Constructs the handler and initializes its OpenEphyra
		 * object.
		 */
		public SyncQAServiceHandler() {
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
		@Override
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
	    @Override
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
	}

	public static class AsyncQAServiceHandler implements LucidaService.AsyncIface {
		private SyncQAServiceHandler handler;

		public AsyncQAServiceHandler() {
			handler = new SyncQAServiceHandler();
		}

		@Override
		public void create(String LUCID, QuerySpec spec, AsyncMethodCallback resultHandler)
				throws TException {
			print("Async Create");
			handler.create(LUCID, spec); // FIXME
			resultHandler.onComplete(null);
		}

		@Override
		public void learn(String LUCID, QuerySpec knowledge, AsyncMethodCallback resultHandler)
				throws TException {
			print("Async Learn");
			handler.learn(LUCID, knowledge); // FIXME
			resultHandler.onComplete(null);
		}

		@Override
		public void infer(String LUCID, QuerySpec query, AsyncMethodCallback resultHandler)
				throws TException {
			print("Async Infer");
			resultHandler.onComplete(handler.infer(LUCID, query));
		}
	}
}
