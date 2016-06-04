package calendar;

import java.util.List;
import java.io.File;
import java.util.ArrayList;

import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;

import thrift.*;

/** 
 * Implementation of the calendar interface. A client request to any
 * method defined in the thrift file is handled by the
 * corresponding method here.
 */
public class CAServiceHandler {
	public static void print(String s) {
		synchronized (System.out) {
			System.out.println(s);
		}
	}

	public static class SyncCAServiceHandler implements LucidaService.Iface {
		/** Text processor for parsing the input query. */
		private TextProcessor TEXT_PROCESSOR;
		
		/** Constructs the handler and initializes its TextProcessor
		 * object.
		 */
		public SyncCAServiceHandler() {
			TEXT_PROCESSOR = new TextProcessor();
		}
		
		/**
		 * Do nothing.
		 * @param LUCID ID of Lucida user
		 * @param spec spec
		 */
		@Override
	    public void create(String LUCID, QuerySpec spec) {}

		/**
		 * Do nothing.
		 * @param LUCID ID of Lucida user
		 * @param knowledge knowledge
		 */
		@Override
	    public void learn(String LUCID, QuerySpec knowledge) {}

	    /**
	     * Extracts a query string and returns the answer from TextProcessor.
		 * @param LUCID ID of Lucida user
		 * @param query query
	     */
	    @Override
	    public String infer(String LUCID, QuerySpec query) {
	    	print("@@@@@ Infer; User: " + LUCID);

	    	if (query.content.isEmpty() || query.content.get(0).data.isEmpty()) {
	    		throw new IllegalArgumentException();
	    	}

	    	String query_data = query.content.get(0).data.get(0);

	    	print("Asking: " + query_data);

	    	String[] time_interval = TEXT_PROCESSOR.parse(query_data);

	    	print("Result " + time_interval[0] + " " + time_interval[1]);

	    	return time_interval[0] + " " + time_interval[1];


	    }
	}

	public static class AsyncCAServiceHandler implements LucidaService.AsyncIface {
		private SyncCAServiceHandler handler;

		public AsyncCAServiceHandler() {
			handler = new SyncCAServiceHandler();
		}

		@Override
		public void create(String LUCID, QuerySpec spec, AsyncMethodCallback resultHandler)
				throws TException {
			print("Async Create");
		}

		@Override
		public void learn(String LUCID, QuerySpec knowledge, AsyncMethodCallback resultHandler)
				throws TException {
			print("Async Learn");
		}

		@Override
		public void infer(String LUCID, QuerySpec query, AsyncMethodCallback resultHandler)
				throws TException {
			print("Async Infer");
			resultHandler.onComplete(handler.infer(LUCID, query));
		}
	}
}
