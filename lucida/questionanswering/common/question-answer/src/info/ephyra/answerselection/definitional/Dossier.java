package info.ephyra.answerselection.definitional;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Properties;

/**
 * A <code>Dossier</code> specifies different properties of a target (such as a
 * person or an event). These properties depend on the target type. For
 * instance, a person has a property 'nationality', whereas an event has a
 * property 'date'.
 * 
 * @author Guido Sautter
 * @version 2008-02-10
 */
public class Dossier {
	protected String target;
	protected String targetType;
	
	private String[] propertyNames = new String[0];
	private Properties properties = new Properties();
	
	/**	
	 * @param 	target		the target String
	 * @param 	targetType	the type of the target
	 */
	public Dossier(String target, String targetType) {
		this.target = target;
		this.targetType = targetType;
		this.propertyNames = getPropertiesForTargetType(this.targetType);
	}
	
	/**
	 * @return	the target.
	 */
	public String getTarget() {
		return this.target;
	}
	
	/**
	 * @return	the targetType.
	 */
	public String getTargetType() {
		return this.targetType;
	}
	
	/**	set a property of the target to a given value
	 * @param	property	the name of the property
	 * @param	value		the value of the property
	 */
	public void setProperty(String property, String value) {
		this.properties.setProperty(property, value);
	}
	
	/**	retrieve the value of a property
	 * @param	property	the name of the property
	 * @return the value associated with the specified property, or null, if there is no such value
	 */
	public String getProperty(String property) {
		return this.properties.getProperty(property);
	}
	
	/**	check if this dossier already contains a value for some property
	 * @param	property	the name of the property to check
	 * @return true iff this dossier has a value associated with the specified property
	 */
	public boolean isPropertySet(String property) {
		return this.properties.containsKey(property);
	}
	
	/**	@return the names of the properties contained in this dossier (regardless if they already have a value associated with them or not)
	 */
	public String[] getPropertyNames() {
		return this.propertyNames;
	}
	
	/**	@return	the names of the properties in this dossier that do not yet have a value associated with them
	 */
	public String[] getMissingPropertyNames() {
		ArrayList<String> list = new ArrayList<String>();
		for (int n = 0; n < this.propertyNames.length; n++)
			if (!this.properties.containsKey(this.propertyNames[n]))
				list.add(this.propertyNames[n]);
		return list.toArray(new String[list.size()]);
	}
	
	/**	retrieve the names of the properties interesting for some given type of target 
	 * @param	targetType	the target type
	 * @return an array containing the names of the properties interesting for a target of the specified type 
	 */
	protected static String[] getPropertiesForTargetType(String targetType) {
		String[] props = targetTypeProperties.get(targetType);
		return (props == null) ? new String[0] : props;
	}
	
	/**	determine the type of a target, using properties as evidence
	 * @param	givenProperties		the names of the properties already known
	 * @return the type of target that is most likely to ahe the specified properties
	 */
	public static String getTargetType(String[] givenProperties) {
		//	determine type making use of the properties already known
		String[] types = targetTypes.toArray(new String[targetTypes.size()]);
		String[][] typeProps = new String[types.length][];
		for (int t = 0; t < types.length; t++) typeProps[t] = getPropertiesForTargetType(types[t]);
		int[] scores = new int[types.length];
		for (int i = 0; i < scores.length; i++) scores[i] = 0;
		
		HashSet<String> propSet = new HashSet<String>();
		for (int p = 0; p < givenProperties.length; p++) propSet.add(givenProperties[p].toLowerCase());
		
		for (int t = 0; t < types.length; t++)
			for (int p = 0; p < typeProps[t].length; p++)
				if (propSet.contains(typeProps[t][p].toLowerCase()))
					scores[t]++;
		
		int max = 0;
		int typeIndex = 0;
		for (int t = 0; t < types.length; t++) {
			if (scores[t] > max) {
				max = scores[t];
				typeIndex = t;
			}
		}
		
		//	FACTOID questions could probably not be interpreted (no properties given), often happens with somewhat wired questions on events
		if (max == 0) return EVENT;
		
		//	return type most likely to have given properties
		return types[typeIndex];
	}
	
	/**	produce a dossier
	 * @param	target				the target string
	 * @param	targetType			the target type (if set to null, the target type will be determined automatically from the given properties)
	 * @param	givenProperties		the properties of the target that are properties already known
	 * @param	givenValues			the values associated with the known properties
	 * @return dossier
	 */
	public static Dossier getDossier(String target, String targetType, String[] givenProperties, String[] givenValues) {
		String type = ((targetType == null) ? getTargetType(givenProperties) : targetType);
		Dossier dossier = new Dossier(target, type);
		for (int p = 0; p < givenProperties.length; p++)
			dossier.setProperty(givenProperties[p], givenValues[p]);
		
		return dossier;
	}
	
	/**	register an additional type of target (hard coded are PERSON, ORGANIZATION, EVENT, ENTERTAINMENT and THING)
	 * @param	targetType	the name for the new target type
	 * @param	properties	the properties that are of interes with regard to the new target type
	 */
	public static void addTargetType(String targetType, String[] properties) {
		if (targetTypeProperties.containsKey(targetType)) {
			HashSet<String> propSet = new HashSet<String>();
			
			String[] props = targetTypeProperties.get(targetType);
			for (int p = 0; p < props.length; p++) propSet.add(props[p]);
			
			for (int p = 0; p < properties.length; p++) propSet.add(properties[p]);
			
			props = propSet.toArray(new String[propSet.size()]);
			targetTypeProperties.put(targetType, props);
		} else {
			targetTypes.add(targetType);
			targetTypeProperties.put(targetType, properties);
		}
	}
	
	/**	add a property to some existing target type
	 * @param	targetType	the target type
	 * @param	property	the new property to be associated with the specified target type
	 */
	public static void addTargetTypeProperty(String targetType, String property) {
		String[] props = targetTypeProperties.get(targetType);
		if (props != null) {
			HashSet<String> propSet = new HashSet<String>();
			
			for (int p = 0; p < props.length; p++) propSet.add(props[p]);
			
			propSet.add(property);
			
			props = propSet.toArray(new String[propSet.size()]);
			targetTypeProperties.put(targetType, props);
		}
	}
	
	private static ArrayList<String> targetTypes = new ArrayList<String>();
	private static HashMap<String, String[]> targetTypeProperties = new HashMap<String, String[]>();
	
	public static final String PERSON = "PERSON";
	public static final String ORGANIZATION = "ORGANIZATION";
	public static final String EVENT = "EVENT";
	public static final String THING = "THING";
	public static final String ENTERTAINMENT = "ENTERTAINMENT";
	
	private static final String[] PERSON_PROPERTIES = {
		"AGE", 
		"ANCESTOR", 
		"CAUSEOFDEATH", 
		"DATEOFBIRTH", 
		"DATEOFDEATH", 
		"DATEOFLIVING", 
		"DATEOFMARRIAGE", 
		"FOOD", 
		"HABITAT", 
		"HEIGHT", 
		"IDENTITY", 
		"INCOME", 
		"KILLER", 
		"LANGUAGE", 
		"LIFESPAN", 
		"NAME", 
		"NATIONALITY", 
		"PLACEOFBIRTH", 
		"PLACEOFDEATH", 
		"PLACEOFLIVING", 
		"PROFESSION", 
		"SIZE", 
		"SPECIALTY"
		};
	private static final String[] ORGANIZATION_PROPERTIES = {
		"ABBREVIATION", 
//		"ACTOR", 
		"AGE", 
		"DATEOFFOUNDATION", 
		"DATEOFSTARTOFOPERATION", 
		"FOUNDER", 
		"FUNCTION", 
		"HABITAT", 
		"INCOME", 
		"LANGUAGE", 
		"LEADER", 
		"LONGFORM", 
		"NAME", 
		"NATIONALITY", 
		"OWNER", 
		"PLACEOFORIGIN", 
		"SPECIALTY", 
		"SYNONYM", 
		"VALUE"
		};
	private static final String[] EVENT_PROPERTIES = {
//		"ABBREVIATION", 
		"ACTOR", 
//		"AGE", 
		"CAUSE", 
		"CONSEQUENCE", 
		"DATE", 
//		"DATEOFCREATION", 
//		"DATEOFEND", 
//		"DATEOFSTART", 
//		"DURATION", 
//		"HABITAT", 
//		"LANGUAGE", 
//		"NAME", 
		"PLACE", 
//		"SYNONYM", 
//		"VALUE", 
//		"WINNER"
		};
	private static final String[] THING_PROPERTIES = {
		"ABBREVIATION", 
		"AGE", 
		"AUTHOR", 
		"BUILDER", 
		"CAPITAL", 
		"DATEOFCREATION", 
		"DATEOFINVENTION",
		"DATEOFSTARTOFOPERATION", 
		"DEFINITION", 
		"DISCOVERER", 
		"DISCOVERY", 
		"DISTANCE", 
		"EXAMPLE", 
		"FOOD", 
		"FUNCTION", 
		"HEIGHT", 
		"INSTRUMENT", 
		"INVENTOR", 
		"MEDICINE", 
		"NAME", 
		"OWNER", 
		"PLACEOFOCCURRENCE", 
		"PLACEOFORIGIN", 
		"POPULATION", 
		"PROVIDER", 
		"RESOURCE", 
		"SIZE", 
		"SPECIES", 
		"SPEED", 
		"SYMPTOM", 
		"SYNONYM", 
		"TEMPERATURE", 
		"VALUE", 
		"WIDTH"
		};
	private static final String[] ENTERTAINMENT_PROPERTIES = {
		"ACTOR", 
		"AGE", 
		"AUTHOR", 
		"DATEOFCREATION", 
		"DATEOFEND", 
		"DATEOFSTART", 
		"DURATION", 
		"EXAMPLE", 
		"INSTRUMENT", 
		"LANGUAGE", 
		"MOVIE", 
		"NAME", 
		"OWNER", 
		"PLACE", 
		"PLACEOFORIGIN", 
		"SYNONYM", 
		"VALUE"
		};
	
	static {
		targetTypes.add(PERSON);
		targetTypeProperties.put(PERSON, PERSON_PROPERTIES);
		targetTypes.add(ORGANIZATION);
		targetTypeProperties.put(ORGANIZATION, ORGANIZATION_PROPERTIES);
		targetTypes.add(EVENT);
		targetTypeProperties.put(EVENT, EVENT_PROPERTIES);
		targetTypes.add(THING);
		targetTypeProperties.put(THING, THING_PROPERTIES);
		targetTypes.add(ENTERTAINMENT);
		targetTypeProperties.put(ENTERTAINMENT, ENTERTAINMENT_PROPERTIES);
	}
}
