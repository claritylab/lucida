package calendar;

import java.util.Date;
import java.io.File;
import java.io.FileNotFoundException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Properties;
import java.util.Scanner;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.Calendar;
import java.text.SimpleDateFormat;

import calendar.CAServiceHandler;
import edu.stanford.nlp.ling.CoreAnnotations;
import edu.stanford.nlp.pipeline.*;
import edu.stanford.nlp.time.*;
import edu.stanford.nlp.time.SUTime.Temporal;
import edu.stanford.nlp.time.SUTime.Time;
import edu.stanford.nlp.util.CoreMap;

/** 
* Parses the query as a range of date.
*/
public class TextProcessor {
	List<AnnotationPipeline> pieplines;

	/** 
	 *  Constructor.
	 *  Builds two pipelines, one for simple date and time, the other for duration.
	 */
	public TextProcessor() {
		pieplines = new ArrayList<AnnotationPipeline>();
		Properties props1 = new Properties();
		addPipeline(props1);
		Properties props2 = new Properties();
		props2.setProperty("sutime.binders", "0");
		props2.setProperty("sutime.markTimeRanges", "true");
		props2.setProperty("sutime.includeRange", "true");
		addPipeline(props2);		
	}

	/** 
	 *  Creates and initializes an annotation pipeline. 
	 *
	 *  @param props Properties describing the pipeline
	 */
	private void addPipeline(Properties props) {
		AnnotationPipeline pipeline = new AnnotationPipeline();
		pipeline.addAnnotator(new TokenizerAnnotator(false));
		pipeline.addAnnotator(new WordsToSentencesAnnotator(false));
		pipeline.addAnnotator(new POSTaggerAnnotator(false));
		pipeline.addAnnotator(new TimeAnnotator("sutime", props));
		pieplines.add(pipeline);
	}

	/** 
	 *  Returns the date range string by parsing text. 
	 *
	 *  @param text String representing the original query
	 */
	public String[] parse(String text) {
		// The pipelines may produce same results, so store results in a set.
		TreeSet<Temporal> has_seen = new TreeSet<Temporal>(new TemporalComparator());
		// Time is comparable, so add temporal of type time and date into a TreeSet to get
		// the minimum and maximum time which define the range for event retrieval.
		TreeSet<Time> times = new TreeSet<Time>();
		for (AnnotationPipeline pipeline : pieplines) {
			Annotation annotation = new Annotation(text);
			annotation.set(CoreAnnotations.DocDateAnnotation.class,
					new SimpleDateFormat("yyyy-MM-dd").format(new Date()));
			pipeline.annotate(annotation);
			List<CoreMap> timexAnnsAll = annotation.get(TimeAnnotations.TimexAnnotations.class);
			for (CoreMap cm : timexAnnsAll) {
				Temporal temporal = cm.get(TimeExpression.Annotation.class).getTemporal();
				temporal.getTime();
				if (has_seen.contains(temporal)) {
					continue;
				}
				has_seen.add(temporal);
				if (temporal.getTimexType().name().equals("TIME")
						|| temporal.getTimexType().name().equals("DATE")) {
					if (temporal.getTime() != null) {
						try {
							times.add(temporal.getTime());
						} catch (NullPointerException e) {}
					}
				}
			}
		}
		// Get the minimum and maximum time only if there are at least two Time objects in times. 
		if (times.size() >= 2) {
			return new String[] {regexNormalize(Collections.min(times).toString(), 0),
					regexNormalize(Collections.max(times).toString(), 1)};
		} 
		// Since the range couldn't be defined by times, define the range from has_seen.
		for (Temporal temporal : has_seen) {
			// Due to a bug (?) in coreNLP, getRange() for "current week" will result in year 2015.
			// Thus, try parsing as week before getRange().
			String[] try_parse_as_week = parseAsWeek(temporal.toString(), text);
			if (try_parse_as_week != null) {
				return try_parse_as_week;
			}
			if (isReadbleTime(temporal.getRange().toString())) {
				List<String> string_list = Arrays.asList(temporal.getRange().toString().split(","));
				String s1 = regexNormalize(string_list.get(0), 0);
				String s2 = regexNormalize(string_list.get(1), 1);
				if (s1.length() >= 10 && s2.length() >= 10 && s1.substring(0, 10).equals(
					s2.substring(0, 10))) {
					if (text.contains("from") || text.contains("start") || text.contains("begin")) {
						s2 = null;
					} else if (text.contains("until")) {
						s1 = null;
					}
				}
				return new String[] {s1, s2};
			}
		}
		// No temporal expression is found by any pipeline.
		return new String[] {null, null};
	}

	/** 
	 *  Returns the date range string by parsing s as a week. 
	 *
	 *  @param s String representing the result of the annotation pipeline
	 *  @param text String representing the original query
	 */
	private String[] parseAsWeek(String s, String text) {
		// Check whether s is like "2016-W23" representing the previous week (a bug in coreNLP?).
		Pattern pattern_week = Pattern.compile("[0-9]{4,4}-[0-9]{2,2}-W[0-9]{2,2}");
		Matcher matcher_week = pattern_week.matcher(s);
		if (matcher_week.find()) {
			String[] s_parsed = matcher_week.group(0).split("W");
			int week_num = Integer.parseInt(s_parsed[s_parsed.length - 1]);
			return new String[] {getDateFromWeek(week_num + 1), getDateFromWeek(week_num + 2)};
		}
		return null;
	}

	/** 
	 *  Returns the date string represented by week_num. 
	 *
	 *  @param week_num int indicating the index of the week in the current year
	 */
	private String getDateFromWeek(int week_num) {
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
		Calendar cal = Calendar.getInstance();
		cal.set(Calendar.WEEK_OF_YEAR, week_num);        
		cal.set(Calendar.DAY_OF_WEEK, Calendar.MONDAY);
		return sdf.format(cal.getTime()).toString() + "T00:00:00";
	}

	/** 
	 *  Returns true if s is a readable string representing time. 
	 *
	 *  @param s String to check
	 */
	private boolean isReadbleTime(String s) {
		return !s.contains("UNKNOWN") && !s.contains("PXW") && !s.contains("PXD");
	}

	/** 
	 *  Returns the normalized form for s and i. 
	 *
	 *  @param s String representing the result of the annotation pipeline
	 *  @param s int representing whether s is the begin (i == 0) or end (i == 1) of the range
	 */
	private static String regexNormalize(String s, int i) {
		String rtn = "";
		Pattern pattern_data = Pattern.compile("[0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}");
		Matcher matcher_date = pattern_data.matcher(s);
		if (matcher_date.find()) {
			rtn += matcher_date.group(0);
		} else {
			rtn += new SimpleDateFormat("yyyy-MM-dd").format(new Date());
		}
		Pattern pattern_time = Pattern.compile("[0-9]{2,2}:[0-9]{2,2}");
		Matcher matcher_time = pattern_time.matcher(s);
		rtn += "T";
		if (matcher_time.find()) {
			rtn += matcher_time.group(0);
			rtn += ":00";
		} else if (s.contains("MO")) {
			rtn += "05:00:00";
		} else if (s.contains("AF")) {
			rtn += "15:00:00";
		} else if (s.contains("NI")) {
			rtn += "18:00:00";
		} else if (i == 0) {
			rtn += "00:00:00";
		} else if (i == 1) {
			rtn += "23:59:59";
		} else {}
		return rtn; // no time zone offset returned
	}

	/**
	 * Main.
	 *  @param args Strings to interpret
	 */
	public static void main(String[] args) {
		TextProcessor p = new TextProcessor();
		boolean debug = false;
		if (!debug) {
			Scanner s = null;
			try {
				s = new Scanner(new File(System.getenv("LUCIDAROOT") +
					"/commandcenter/data/class_CA.txt"));
			} catch (FileNotFoundException e) {
				e.printStackTrace();
				return;
			}
			ArrayList<String> lines = new ArrayList<String>();
			while (s.hasNextLine()){
				lines.add(s.nextLine());
			}
			s.close();
			for (String line : lines) {
				System.out.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ " + line);
				for (String result : p.parse(line)) {
					System.out.println(result);
				}
			}
		} else {
			for (String result : p.parse(args[0])) {
				System.out.println(result);
			}
		}
	}

	/** 
	 *  Compares two Temporals. 
	 */
	private static class TemporalComparator implements Comparator<Temporal> {
		@Override
		public int compare(Temporal o1, Temporal o2) {
			int time_val_diff = o1.toString().compareTo(o2.toString());
			if (time_val_diff != 0) {
				return time_val_diff;
			}
			int type_diff = o1.getTimexType().name().compareTo(o2.getTimexType().name());
			return type_diff;
		}
	}
}
