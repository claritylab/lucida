package info.ephyra.questionanalysis.atype.minorthird.hierarchical;

import java.io.Serializable;

import edu.cmu.minorthird.classify.ClassLabel;
import edu.cmu.minorthird.classify.Classifier;
import edu.cmu.minorthird.classify.Explanation;
import edu.cmu.minorthird.classify.Instance;

/**
 * Dummy classifier that always assigns the same class.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class DummyClassifier implements Classifier,Serializable{
	
	private static final long serialVersionUID=1;
	
	private String soleLabelName;
	
	public DummyClassifier(String soleLabelName){
		this.soleLabelName=soleLabelName;
	}

	public ClassLabel classification(Instance instance){
		return new ClassLabel(soleLabelName);
	}

	public String explain(Instance instance){
		return "Always classify instance as "+soleLabelName;
	}

	public Explanation getExplanation(Instance instance){
		return new Explanation(explain(instance));
	}

}
