package info.ephyra.questionanalysis.atype.minorthird.hierarchical;

import edu.cmu.minorthird.classify.Classifier;
import edu.cmu.minorthird.classify.ClassifierLearner;
import edu.cmu.minorthird.classify.Example;
import edu.cmu.minorthird.classify.ExampleSchema;
import edu.cmu.minorthird.classify.Instance;
import edu.cmu.minorthird.classify.Instance.Looper;

/**
 * Dummy learner that simply returns a classifier which always assigns the same
 * class.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class DummyClassifierLearner implements ClassifierLearner{
	
	String soleLabelName;
	
	public DummyClassifierLearner(String soleLabelName){
		this.soleLabelName=soleLabelName;
	}

	public void addExample(Example example){
	}

	public void completeTraining(){
	}

	public ClassifierLearner copy(){
		return this;
	}

	public Classifier getClassifier(){
		return new DummyClassifier(soleLabelName);
	}

	public boolean hasNextQuery(){
		return false;
	}

	public Instance nextQuery(){
		return null;
	}

	public void reset(){
	}

	public void setInstancePool(Looper instancePool){
	}

	public void setSchema(ExampleSchema schema){
	}

}
