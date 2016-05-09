package lucida.handler;

// Java packages
import java.util.List;
import java.util.ArrayList;

import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;

// Interface definition
import lucida.thrift.*;

public class QAServiceHandler {
	private static void print(String s) {
		synchronized (System.out) {
			System.out.println(s);
		}
	}

	public static class SyncQAServiceHandler implements LucidaService.Iface {
		Integer count = 0;

		@Override
		public void create(String LUCID, QuerySpec spec) {
			print("Sync Create");
		}

		@Override
		public void learn(String LUCID, QuerySpec knowledge) {
			print("Sync Learn");
		}

		@Override
		public String infer(String LUCID, QuerySpec query) {
			print("Sync Infer");
			int rtn = 0;
			synchronized (count) {
				rtn = count++;
			}
			return rtn + " QA says I love XXX";
		}	
	}

	public static class AsyncQAServiceHandler implements LucidaService.AsyncIface {
		private SyncQAServiceHandler handler;

		public AsyncQAServiceHandler() {
			handler = new SyncQAServiceHandler();
		}

		@Override
		public void create(String LUCID, QuerySpec spec, AsyncMethodCallback resultHandler) throws TException {
			print("Async Create");
		}

		@Override
		public void learn(String LUCID, QuerySpec knowledge, AsyncMethodCallback resultHandler) throws TException {
			print("Async Learn");
		}

		@Override
		public void infer(String LUCID, QuerySpec query, AsyncMethodCallback resultHandler) throws TException {
			print("Async Infer");
			try {
				print("Sleeping for 3 seconds");
				Thread.sleep(3000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			resultHandler.onComplete(handler.infer(LUCID, query));
		}
	}
}

