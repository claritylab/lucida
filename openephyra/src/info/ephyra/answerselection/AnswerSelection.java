package info.ephyra.answerselection;

import info.ephyra.answerselection.filters.Filter;
import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;

import java.util.ArrayList;
import java.util.concurrent.*;

/**
 * <p>The <code>AnswerSelection</code> component applies <code>Filters</code> to
 * <code>Results</code> to promote promising results, to drop results that are
 * unlikely to answer the question and to derive additional results from the raw
 * results returned by the <code>Searchers</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2006-06-28
 */
public class AnswerSelection {
	/**
	 * The <code>Filters</code> that are applied to the <code>Results</code>.
	 * Filters are applied in the order in which they appear in this list.
	 */
	private static ArrayList<Filter> filters = new ArrayList<Filter>();
	
	/**
	 * Registers a <code>Filter</code>. Filters are applied in the order in
	 * which they are registered.
	 * 
	 * @param filter <code>Filter</code> to add
	 */
	public static void addFilter(Filter filter) {
		filters.add(filter);
	}
	
	/**
	 * Unregisters all <code>Filters</code>.
	 */
	public static void clearFilters() {
		filters.clear();
	}

    public static void parFilters(final Filter filter, final Result[] results) {
        final int threads = Runtime.getRuntime().availableProcessors();
        ExecutorService service = Executors.newFixedThreadPool(threads);
        
        final int chunk = results.length / threads;
        int offset = 0;
        for (final Result result : results) {
            for (int i = 0; i < threads; ++i) {
                final int start = offset;
                final int end = offset + chunk;
                service.execute(new Runnable() {

                    @Override
                    public void run() {

                        /* for (int i = start; i < end; ++i) */
                        /*     filter.apply(result[i]); */
                    }
                });
                offset += chunk;
            }
            service.shutdown();
            try {
                service.awaitTermination(10, TimeUnit.SECONDS);
            } catch (InterruptedException ignore) {
                System.out.println("Not sure what this is.");
            }

            /* for(int i = results.length - chunk; i < results.length; ++i) */
            /*     filter.apply(results[i]); */
        }
    }

	/**
	 * Applies <code>Filters</code> to the <code>Results</code> from the search
	 * component and returns up to <code>maxResults</code> results with a score
	 * of at least <code>minScore</code>.
	 * 
	 * @param results search results
	 * @param maxResults maximum number of results to be returned
	 * @param minScore minimum score of a result that is returned
	 * @return up to <code>maxResults</code> results
	 */
	public static Result[] getResults(Result[] results, int maxResults,
									  float minScore) {
		// apply filters
		for (Filter filter : filters) {
            int before = results.length;
			results = filter.apply(results);
            int after = results.length;
            if(before != after){
                MsgPrinter.printFilterStarted(filter, before);
                MsgPrinter.printFilterFinished(filter, after);
            }
		}
		/* for (Filter filter : filters) { */
        /*     parFilters(filter, results); */
		/* } */
		
		// get up to maxResults results with a score of at least minScore
		ArrayList<Result> resultsList = new ArrayList<Result>();
		for (Result result : results) {
			if (maxResults == 0) break;
			
			if (result.getScore() >= minScore) {
				resultsList.add(result);
				maxResults--;
			}
		}
		
		return resultsList.toArray(new Result[resultsList.size()]);
	}
}
