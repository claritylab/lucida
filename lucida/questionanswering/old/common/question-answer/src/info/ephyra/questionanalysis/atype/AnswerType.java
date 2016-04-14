package info.ephyra.questionanalysis.atype;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * A basic class which contains hierarchical answer type information.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class AnswerType implements Serializable
{
    private static final long serialVersionUID = 20061012L; // YYYYMMDD
    // only thread-safe static objects!!
//  private static final Logger log = Logger.getLogger(AnswerType.class);

    /**
     * the level in an answer type hierarchy, 1 being the top level answer type,
     * 2 being the first subtype level, etc
     */
    protected int level;
    protected double confidence;
    protected String type; // e.g. NUMEX, CARDINAL, etc.
    protected List<AnswerType> subtypes = new ArrayList<AnswerType>(1);

    public String toString()
    {
        return getFullType(-1)+" ("+confidence+")";
    }

    /**
     * Constructs an Answer Type object. Note that no default constructor is
     * provided
     * @param level A int indicating the level of this answer type in answer
     * type hierarchy
     * @param confidence A double indicating the confidence score
     * @param type A String indicating the type
     */
    public AnswerType(int level, double confidence, String type)
    {
        this.level = level;
        this.confidence = confidence;
        this.type = type;
    }

    /**
     * Constructs an Answer Type object with pre-defined list of subtypes.
     * @param level A int indicating the level of this answer type in answer
     * type hierarchy
     * @param confidence A double indicating the confidence score
     * @param type A String indicating the type
     * @param subtypes A List containing subtype information about the current
     * answer type
     */
    public AnswerType(int level,
                      double confidence,
                      String type,
                      List<AnswerType> subtypes)
    {
        this.level = level;
        this.confidence = confidence;
        this.type = type;
        this.subtypes = subtypes;
    }

    public boolean equals(Object obj){
      if(!(obj instanceof AnswerType))
        return false;
      AnswerType atype = (AnswerType)obj;
      if(!this.type.equals(atype.type))
        return false;
      if(this.level != atype.level)
        return false;
      if(this.subtypes.size() != atype.subtypes.size())
        return false;
      List<AnswerType> asubtypes = atype.getSubtypes();  
      for(int i=0;i<this.subtypes.size();i++)
        if(!this.subtypes.get(i).equals(asubtypes.get(i)))
          return false;
      return true;    
    }
    
    public int hashCode() {
        return getFullType(-1).hashCode();
    }

    /**
     * @return the level of this answer type in answer type hierarchy
     */
    public int getLevel()
    {
        return this.level;
    }

    /**
     * @return boolean indicating if the current answer type is top-level
     */
    public boolean isTopLevel()
    {
        return (this.level == 1);
    }

    /**
     * Sets the Answer Type level
     * @param level the answer type level
     */
    public void setLevel(int level)
    {
        this.level = level;
    }

    public String getType()
    {
        return this.type;
    }

    public double getConfidence()
    {
        return this.confidence;
    }

    public List<AnswerType> getSubtypes()
    {
        return this.subtypes;
    }

    public void setConfidence(double confidence)
    {
        this.confidence = confidence;
    }

    public void setSubtypes(List<AnswerType> subtypes)
    {
        this.subtypes = subtypes;
    }
    
    public String getFullType(int classLevels) {
        if (classLevels == -1) classLevels = subtypes.size()+1;
        String fullType = this.type;
        for (int i = 0; i < subtypes.size() && i < classLevels-1; i++) {
            AnswerType subtype = subtypes.get(i);
            fullType += "."+subtype.getType();
        }
        return fullType;
    }

    /**
     * Construct answer type from a String that looks like NUMEX.MONEY
     * <p>This string cannot have multiple types at any level 
     * (e.g NUMEX|PERSON not allowed) Any number of "."-separated subtypes 
     * can be present. Subtypes called DEFAULT are ignored.
     * 
     * @param answerTypeStr the String from which to construct an AnswerType
     */
    public static AnswerType constructFromString(String answerTypeStr){
        String[] atypes = answerTypeStr.split("\\.");
        List<AnswerType> subtypes = new ArrayList<AnswerType>(1);
        if (atypes.length > 1) {
            for (int i=1;i<atypes.length;i++) {
                if (!atypes[i].equals("DEFAULT"))
                    subtypes.add(new AnswerType(i+1,1.0,atypes[i]));
            }
        }
        return new AnswerType(1,1.0,atypes[0],subtypes);
    }
}
