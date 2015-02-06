package edu.cmu.sphinx.util.props;

import org.xml.sax.Attributes;
import org.xml.sax.Locator;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.logging.Logger;

/** A SAX XML Handler implementation that builds up the map of raw property data objects */
public class ConfigHandler extends DefaultHandler {

    protected RawPropertyData rpd;
    protected Locator locator;
    protected List<String> itemList;
    protected String itemListName;
    protected StringBuilder curItem;

    protected final Map<String, RawPropertyData> rpdMap;
    protected final Map<String, String> globalProperties;

    private boolean replaceDuplicates;
    private final URL baseURL;

    public ConfigHandler(Map<String, RawPropertyData> rpdMap, Map<String, String> globalProperties,
                         boolean replaceDuplicates, URL baseURL) {
        this.rpdMap = rpdMap;
        this.globalProperties = globalProperties;
        this.replaceDuplicates = replaceDuplicates;
        this.baseURL = baseURL;
    }

    public ConfigHandler(Map<String, RawPropertyData> rpdMap, Map<String, String> globalProperties) {
        this(rpdMap, globalProperties, false, null);
    }

    @Override
    public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
        if (qName.equals("config")) {
            // test if this configuration extends another one
            String extendedConfigName = attributes.getValue("extends");
            if (extendedConfigName != null) {
                mergeConfigs(extendedConfigName, true);
                replaceDuplicates = true;
            }
        } else if (qName.equals("include")) {
            String includeFileName = attributes.getValue("file");
            mergeConfigs(includeFileName, false);
        } else if (qName.equals("extendwith")) {
            String includeFileName = attributes.getValue("file");
            mergeConfigs(includeFileName, true);
        } else if (qName.equals("component")) {
            String curComponent = attributes.getValue("name");
            String curType = attributes.getValue("type");
            if (rpdMap.get(curComponent) != null && !replaceDuplicates) {
                throw new SAXParseException("duplicate definition for " + curComponent, locator);
            }
            rpd = new RawPropertyData(curComponent, curType);
        } else if (qName.equals("property")) {
            String name = attributes.getValue("name");
            String value = attributes.getValue("value");
            if (attributes.getLength() != 2 || name == null || value == null) {
                throw new SAXParseException("property element must only have 'name' and 'value' attributes", locator);
            }
            if (rpd == null) {
                // we are not in a component so add this to the global
                // set of symbols
//                    String symbolName = "${" + name + "}"; // why should we warp the global props here
                globalProperties.put(name, value);
            } else if (rpd.contains(name) && !replaceDuplicates) {
                throw new SAXParseException("Duplicate property: " + name, locator);
            } else {
                rpd.add(name, value);
            }
        } else if (qName.equals("propertylist")) {
            itemListName = attributes.getValue("name");
            if (attributes.getLength() != 1 || itemListName == null) {
                throw new SAXParseException("list element must only have the 'name'  attribute", locator);
            }
            itemList = new ArrayList<String>();
        } else if (qName.equals("item")) {
            if (attributes.getLength() != 0) {
                throw new SAXParseException("unknown 'item' attribute", locator);
            }
            curItem = new StringBuilder();
        } else {
            throw new SAXParseException("Unknown element '" + qName + '\'', locator);
        }
    }

    @Override
    public void characters(char ch[], int start, int length) throws SAXParseException {
        if (curItem != null) {
            curItem.append(ch, start, length);
        }
    }

    @Override
    public void endElement(String uri, String localName, String qName) throws SAXParseException {
        if (qName.equals("component")) {
            rpdMap.put(rpd.getName(), rpd);
            rpd = null;
        } else if (qName.equals("property")) {
            // nothing to do
        } else if (qName.equals("propertylist")) {
            if (rpd.contains(itemListName)) {
                throw new SAXParseException("Duplicate property: " + itemListName, locator);
            } else {
                rpd.add(itemListName, itemList);
                itemList = null;
            }
        } else if (qName.equals("item")) {
            itemList.add(curItem.toString().trim());
            curItem = null;
        }
    }

    @Override
    public void setDocumentLocator(Locator locator) {
        this.locator = locator;
    }

    private void mergeConfigs(String configFileName, boolean replaceDuplicates) {
        try {
            File parent = new File(baseURL.toURI().getPath()).getParentFile();
            URL fileURL = new File(parent.getPath() + File.separatorChar +  configFileName).toURI().toURL();

            Logger logger = Logger.getLogger(ConfigHandler.class.getSimpleName());
            logger.fine((replaceDuplicates ? "extending" : "including") + " config:" + fileURL.toURI());

            SaxLoader saxLoader = new SaxLoader(fileURL, globalProperties, rpdMap, replaceDuplicates);
            saxLoader.load();
        } catch (IOException e) {
            throw new RuntimeException("Error while processing <include file=\"" + configFileName + "\">: " + e, e);
        } catch (URISyntaxException e) {
            e.printStackTrace();
        }
    }
}
