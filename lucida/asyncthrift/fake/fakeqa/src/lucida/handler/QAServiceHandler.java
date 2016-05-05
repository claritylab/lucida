package lucida.handler;

// Java packages
import java.util.List;
import java.util.ArrayList;

import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;

// Interface definition
import lucida.thrift.*;

public class QAServiceHandler {
	public static class SyncQAServiceHandler implements LucidaService.Iface {
		@Override
		public void create(String LUCID, QuerySpec spec) {
			System.out.println("Sync Create");
		}

		@Override
		public void learn(String LUCID, QuerySpec knowledge) {
			System.out.println("Sync Learn");
		}

		@Override
		public String infer(String LUCID, QuerySpec query) {
			System.out.println("Sync Infer");
			return "QA Result: I love XXX";
		}	
	}
	
	public static class AsyncQAServiceHandler implements LucidaService.AsyncIface {
		private SyncQAServiceHandler handler;
		
		public AsyncQAServiceHandler() {
			handler = new SyncQAServiceHandler();
		}
		
		@Override
		public void create(String LUCID, QuerySpec spec, AsyncMethodCallback resultHandler) throws TException {
			System.out.println("Async Create");
		}

		@Override
		public void learn(String LUCID, QuerySpec knowledge, AsyncMethodCallback resultHandler) throws TException {
			System.out.println("Async Learn");
		}

		@Override
		public void infer(String LUCID, QuerySpec query, AsyncMethodCallback resultHandler) throws TException {
			System.out.println("Async Infer");
			try {
				System.out.println("Sleeping for 3 seconds");
				Thread.sleep(3000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			resultHandler.onComplete(handler.infer(LUCID, query));
		}
	}
}

