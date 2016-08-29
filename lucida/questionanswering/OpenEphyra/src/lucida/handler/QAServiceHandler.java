package lucida.handler;

// Open Ephyra packages
import info.ephyra.OpenEphyra;
import info.ephyra.search.Result;
import info.ephyra.io.MsgPrinter;

// Java packages
import java.util.List;
import java.io.File;
import java.util.ArrayList;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;

// Interface definition
import lucida.thrift.*;

/** Implementation of the question-answer interface defined
 * in the question-answer thrift file. A client request to any
 * method defined in the thrift file is handled by the
 * corresponding method here.
 */
public class QAServiceHandler {
	public static class SyncQAServiceHandler implements LucidaService.Iface {
		/** An object that lets the question-answer wrapper use
		 * the end-to-end OpenEphyra framework.
		 */
		private OpenEphyra oe;

		/**
		 * Default answer.
		 */
		private String default_answer;

		/**
		 * Since OpenEphyra is not thread-safe, we enforce the use to be single-threaded.
		 */
		private Lock infer_lock;

		/** Constructs the handler and initializes its OpenEphyra
		 * object.
		 */
		public SyncQAServiceHandler() {
			String dir = "";
			MsgPrinter.enableStatusMsgs(true);
			MsgPrinter.enableErrorMsgs(true);
			// Initialize OE pipeline.
			oe = new OpenEphyra(dir);
			default_answer = "Factoid not found in knowledge base.";
			infer_lock = new ReentrantLock();
			// Create db directory.
			if (!new File("db").exists()) {
				new File("db").mkdir();
			}
		}

		/**
		 * Creates a new OpenEphyra service for user LUCID.
		 * @param LUCID ID of Lucida user
		 * @param spec spec
		 */
		@Override
		public void create(String LUCID, QuerySpec spec) {
			MsgPrinter.printStatusMsg("@@@@@ Create; User: " + LUCID);
		}

		/**
		 * Adds knowledge to OpenEphyra.
		 * @param LUCID ID of Lucida user
		 * @param knowledge knowledge
		 */
		@Override
		public void learn(String LUCID, QuerySpec knowledge) {
			MsgPrinter.printStatusMsg("@@@@@ Learn; User: " + LUCID);
			try {
				KnowledgeBase kb = KnowledgeBase.getKnowledgeBase(LUCID);
				kb.addKnowledge(knowledge);
			} catch (Exception e) {
				e.printStackTrace();
				throw new RuntimeException((e.toString() != null) ? e.toString() : "");
			}
		}

		/**
		 * Extracts a query string and returns the answer from OpenEphyra.
		 * @param LUCID ID of Lucida user
		 * @param query query
		 */
		@Override
		public String infer(String LUCID, QuerySpec query) {
			MsgPrinter.printStatusMsg("@@@@@ Infer; User: " + LUCID);
			String answer = "";
			try {
				infer_lock.lock(); // limit concurrency because OE is not thread-safe
				// Set INDRI_INDEX.
				System.setProperty("INDRI_INDEX",
						KnowledgeBase.getKnowledgeBase(LUCID).getIndriIndex());
				if (query.content.isEmpty() || query.content.get(0).data.isEmpty()) {
					throw new IllegalArgumentException();
				}
				// Only look for the first item in content and data.
				// The rest part of query is ignored.
				answer = askFactoidThrift(LUCID, query.content.get(0).data.get(0));
				MsgPrinter.printStatusMsg("Answer: " + answer);
				// Check if it needs to ask ENSEMBLE.
				if (answer.equals(default_answer) && query.content.get(0).tags.get(2).equals("1")) {
					QuerySpec ENSEMBLE_spec = new QuerySpec();
					ENSEMBLE_spec.name = "query";
					ENSEMBLE_spec.content = new ArrayList<QueryInput>();
					ENSEMBLE_spec.content.add(query.content.get(1));
					String ENSEMBLE_addr = query.content.get(1).tags.get(0);
					int ENSEMBLE_port = Integer.parseInt(query.content.get(1).tags.get(1));
					TTransport transport = new TSocket(ENSEMBLE_addr, ENSEMBLE_port);
					TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
					LucidaService.Client client = new LucidaService.Client(protocol);
					MsgPrinter.printStatusMsg("Asking ENSEMBLE at " + ENSEMBLE_addr + " "
					+ ENSEMBLE_port);
					answer = client.infer(LUCID, ENSEMBLE_spec);
				}
			} catch (Exception e) {
				e.printStackTrace();
				throw new RuntimeException((e.toString() != null) ? e.toString() : "");
			} finally {
				infer_lock.unlock(); // always unlock at the end
			}
			return answer;
		}

		/** Forwards the client's question to the OpenEphyra object's askFactoid
		 * method and collects the response.
		 * @param LUCID ID of Lucida user
		 * @param question eg. "what is the speed of light?"
		 * @throws Exception 
		 */
		private String askFactoidThrift(String LUCID, String question) throws Exception {
			MsgPrinter.printStatusMsg("askFactoidThrift(): Arg = " + question);
			Result result = oe.askFactoid(question);	
			String answer = default_answer;
			if (result != null) {
				answer = result.getAnswer();
			}
			// Check if Wikipedia Indri repository is pre-configured.
			String wiki_indri_index = System.getenv("wiki_indri_index");
			if (wiki_indri_index == null) {
				return answer;
			}
			// Set INDRI_INDEX.
			System.setProperty("INDRI_INDEX", wiki_indri_index);
			result = oe.askFactoid(question);
			if (result != null) {
				answer += " (from Wikipedia: ";
				answer += result.getAnswer();
				answer += ")";
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
			MsgPrinter.printStatusMsg("Async Create");
			handler.create(LUCID, spec); // FIXME
			resultHandler.onComplete(null);
		}

		@Override
		public void learn(String LUCID, QuerySpec knowledge, AsyncMethodCallback resultHandler)
				throws TException {
			MsgPrinter.printStatusMsg("Async Learn");
			handler.learn(LUCID, knowledge); // FIXME
			resultHandler.onComplete(null);
		}

		@Override
		public void infer(String LUCID, QuerySpec query, AsyncMethodCallback resultHandler)
				throws TException {
			MsgPrinter.printStatusMsg("Async Infer");
			resultHandler.onComplete(handler.infer(LUCID, query));
		}
	}
}
