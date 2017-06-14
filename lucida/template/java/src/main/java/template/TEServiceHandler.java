package template;

import java.util.List;
import java.io.File;
import java.util.ArrayList;

import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;

import thrift.*;

/** 
 * Implementation of the template interface. A client request to any
 * method defined in the thrift file is handled by the
 * corresponding method here.
 */
public class TEServiceHandler {
	public static void print(String s) {
		synchronized (System.out) {
			System.out.println(s);
		}
	}

	public static class SyncTEServiceHandler implements LucidaService.Iface {
		/**
		 * TODO: Implement your own create function for your service.
		 * @param LUCID ID of Lucida user
		 * @param spec spec
		 */
		@Override
	    public void create(String LUCID, QuerySpec spec) {}

		/**
		 * TODO: Implement your own learn function for your service.
		 * @param LUCID ID of Lucida user
		 * @param knowledge knowledge
		 */
		@Override
	    public void learn(String LUCID, QuerySpec knowledge) {}

	    /**
	     * TODO: Implement your own infer function for your service.
	     * Here is a sample infer function.
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
	    	String answer_data = "This is the sample answer";
	    	print("Result: " + answer_data);
	    	return answer_data;
	    }
	}

	public static class AsyncTEServiceHandler implements LucidaService.AsyncIface {
		private SyncTEServiceHandler handler;

		public AsyncTEServiceHandler() {
			handler = new SyncTEServiceHandler();
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
