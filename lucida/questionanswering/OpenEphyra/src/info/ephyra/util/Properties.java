package info.ephyra.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Properties class the way it was meant to be; Generics and property filtering
 * and manipulation based on the conventional dot-separator property name
 * syntax.
 * 
 * @author Andy Schlaikjer, Nico Schlaefer
 * @version 2008-02-10
 */
public class Properties extends HashMap<String,String>
{
	private static final long serialVersionUID = 1L;

	public Properties() {
		super();
	}

	public Properties(Properties properties) {
		super(properties);
	}

	public String getProperty(String name) {
		return get(name)!=null?get(name).trim():null;
	}

	public void setProperty(String name, String value) {
		put(name, value);
	}

	public String getProperty(String name, String defaultValue) {
		String value = get(name);
		if (value == null)
			return defaultValue;
		return value;
	}

	public Set<String> getPropertyNames() {
		return keySet();
	}

	/**
	 * @see java.util.Properties#load(InputStream)
	 * @param is
	 * @throws IOException
	 */
	public void load(InputStream is) throws IOException {
		java.util.Properties properties = new java.util.Properties();
		properties.load(is);
		for (Map.Entry entry : properties.entrySet())
			put((String) entry.getKey(), (String) entry.getValue());
	}

	/**
	 * @see java.util.Properties#loadFromXML(InputStream)
	 * @param is
	 * @throws IOException
	 */
	public void loadFromXML(InputStream is) throws IOException {
		java.util.Properties properties = new java.util.Properties();
		properties.loadFromXML(is);
		for (Map.Entry entry : properties.entrySet())
			put((String) entry.getKey(), (String) entry.getValue());
	}

	/**
	 * Filters the entries in this Properties object and returns a new object
	 * containing only those entries whose keys match the given string prefix.
	 * Optionally, you may have the prefix removed from the keys in the returned
	 * Properties object.
	 * 
	 * @param prefix the prefix string to use as a filter on existing entry keys
	 * @param remove_prefix if true, strips the prefix off of all key values in
	 *                      the returned Properties object
	 * @return a new Properties object containing only those entries from this
	 *         Properties object whose keys match the given prefix
	 */
	public Properties filterProperties(String prefix, boolean remove_prefix) {
		Properties properties = new Properties();
		for (Map.Entry<String,String> property : entrySet()) {
			String key = property.getKey();
			if (key.startsWith(prefix)) {
				if (remove_prefix)
					key = key.substring(prefix.length());
				properties.put(key, property.getValue());
			}
		}
		return properties;
	}

	/**
	 * For each key in this Properties object a prefix string is created from
	 * all leading characters before the first '.' (period) character. This
	 * prefix string is used as a new key which will reference a new Properties
	 * object containing the original key stripped of the prefix and the
	 * original value. For example, if this Properties object contains the
	 * following entries:
	 * 
	 * <pre>
	 * one = yes
	 * one.a = 1
	 * one.b = 2
	 * two.a = 3
	 * two.b = 4
	 * </pre>
	 * 
	 * then the returned Map<String,Properties> would look like this (Perl
	 * notation):
	 * 
	 * <pre>
	 * {
	 *    one =&gt; {
	 *       a =&gt; '1',
	 *       b =&gt; '2'
	 *    }, 
	 *    two =&gt; {
	 *       a =&gt; '3',
	 *       b =&gt; '4'
	 *    }
	 * }
	 * </pre>
	 */
	public Map<String,Properties> mapProperties() {
		Map<String,Properties> properties_map = new HashMap<String,Properties>();
		for (Map.Entry<String, String> property: entrySet()) {
			String key = property.getKey();
			int i = key.indexOf('.');
			if (i == -1) {
				Properties properties = properties_map.get(key);
				if (properties == null) {
					properties = new Properties();
					properties_map.put(key, properties);
				}
				properties.put(key, property.getValue());
				continue;
			}
			String name = key.substring(0, i); 
			String property_name = key.substring(i+1);
			Properties properties = properties_map.get(name);
			if (properties == null) {
				properties = new Properties();
				properties_map.put(name, properties);
			}
			properties.put(property_name, property.getValue());
		}
		return properties_map;
	}

	/**
	 * @return a java.util.Properties object containing equivalent key-value
	 *         entries
	 */
	public java.util.Properties toJavaProperties() {
		java.util.Properties properties = new java.util.Properties();
		properties.putAll(this);
		return properties;
	}

	/**
	 * Loads properties from a file given a class name.
	 * 
	 * Sample usage:
	 * <code>
	 * Properties p = 
	 * Properties.loadPropertiesFromClassName(getClass().getName());
	 * </code>
	 * 
	 * @return a Properties object containing the content of the properties file
	 */
	public static Properties loadFromClassName(String className) {
		Properties p = new Properties();
		File userProperties;
		String conf_dir = "conf";
		try {
			userProperties = new File(conf_dir, className + ".properties");
			if (!userProperties.exists()) {
				// if properties file not in conf_dir then search classpath
				URL url = (new Properties()).getClass().getClassLoader()
						.getResource(className + ".properties");
				if (url != null) userProperties = new File(url.getFile());
			}
			if (!userProperties.exists())
				throw new IOException("Missing properties file for " +
						className + "\n" + "Make sure that " + className +
						".properties" + " is in the folder \"" + conf_dir +
						"\" or on the classpath.");
			p.load(new FileInputStream(userProperties));
		} catch (Exception e) {
			System.err.println("Caught exception while loading properties: "
					+ e.getMessage());
		}
		return p;
	}
}
