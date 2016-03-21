package info.ephyra.questionanalysis.atype.minorthird.hierarchical;

import java.io.Serializable;
import java.util.HashMap;

import edu.cmu.minorthird.classify.ClassLabel;
import edu.cmu.minorthird.classify.Classifier;
import edu.cmu.minorthird.classify.Explanation;
import edu.cmu.minorthird.classify.Instance;

/**
 * A hierarchy of classifiers. At first, a top-level classifier is applied.
 * Classifiers for subclasses are selected based on the outcome of previous
 * classifications.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class HierarchicalClassifier implements Classifier,Serializable{
	
	private static final long serialVersionUID=1;

	private HashMap classifiers;
	private int classLevels;

	public HierarchicalClassifier(HashMap classifiers,int classLevels){
		this.classifiers=classifiers;
		this.classLevels=classLevels;
	}

	private String getNewLabelName(String currentClass,String sublabel,int level){
		if(level==0){
			return sublabel;
		}
		else{
			return currentClass+"."+sublabel;
		}
	}

	public ClassLabel classification(Instance instance){
		String labelName="";
		double weight=1;
		for(int i=0;i<classLevels;i++){
			Classifier currentClassifier=(Classifier)classifiers.get(labelName);
			ClassLabel currentLabel=currentClassifier.classification(instance);
			labelName=getNewLabelName(labelName,currentLabel.bestClassName(),i);
			weight*=currentLabel.bestWeight();
		}
		return new ClassLabel(labelName,weight);
	}

	public String explain(Instance instance){
		String labelName="";
		String explanation="";
		for(int i=0;i<classLevels;i++){
			Classifier currentClassifier=(Classifier)classifiers.get(labelName);
			ClassLabel currentLabel=currentClassifier.classification(instance);
			labelName=getNewLabelName(labelName,currentLabel.bestClassName(),i);
			explanation+=currentClassifier.explain(instance);
		}
		return explanation;
	}

	public Explanation getExplanation(Instance instance){
		return new Explanation(explain(instance));
	}

	public static String getHierarchicalClassName(String original,int levels,boolean useClassLevels){
		if(useClassLevels){
			String[] split=original.split("\\.");
			String classLabelName=split[0];
			for(int i=1;i<levels;i++){
				if(split.length<=i){
					classLabelName+=".DEFAULT";
				}
				else{
					classLabelName+="."+split[i];
				}
			}
			return classLabelName;
		}
		else{
			return original.replace('.','-');
		}
	}

}
