package info.ephyra.questionanalysis.atype.minorthird.hierarchical;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import edu.cmu.minorthird.classify.ClassLabel;
import edu.cmu.minorthird.classify.Classifier;
import edu.cmu.minorthird.classify.ClassifierLearner;
import edu.cmu.minorthird.classify.Example;
import edu.cmu.minorthird.classify.ExampleSchema;
import edu.cmu.minorthird.classify.Instance;
import edu.cmu.minorthird.classify.Instance.Looper;

/**
 * Learner for hierarchical classifiers.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class HierarchicalClassifierLearner implements ClassifierLearner{

	private ClassifierLearner[] prototypes;
	private HashMap<String, ClassifierLearner> classifierLearners;

	private Looper instancePool;

	public HierarchicalClassifierLearner(ClassifierLearner[] prototypes){
		this.prototypes=prototypes;
		classifierLearners=new HashMap<String, ClassifierLearner>();
	}

	public void addExample(Example example){
		for(int i=0;i<prototypes.length;i++){
			String labelName=example.getLabel().bestClassName();
			String prefix=getLabelPrefix(labelName,i);
			String sublabel=getSublabel(labelName,i);
			Example subExample=new Example(example.asInstance(),new ClassLabel(sublabel));
			ClassifierLearner subLearner=classifierLearners.get(prefix);
			subLearner.addExample(subExample);
		}
	}

	private String getLabelPrefix(String labelName,int level){
		if(level==0){
			return "";
		}
		else{
			String[] split=labelName.split("\\.");
			String prefix=split[0];
			for(int i=1;i<level;i++){
				prefix+="."+split[i];
			}
			return prefix;
		}
	}

	private String getSublabel(String labelName,int level){
		String[] split=labelName.split("\\.");
		return split[level];
	}

	private static ClassifierLearner[] toValueArray(HashMap<String, ClassifierLearner> learnerHash){
		return learnerHash.values().toArray(new ClassifierLearner[learnerHash.size()]);
	}

	private static String[] toKeyArray(HashMap<String, ClassifierLearner> learnerHash){
		return learnerHash.keySet().toArray(new String[learnerHash.size()]);
	}

	public void completeTraining(){
		System.out.println("Completing training for all...");
		String[] keys=toKeyArray(classifierLearners);
		for(int i=0;i<keys.length;i++){
			System.out.println("Completing training for "+keys[i]+"...");
			classifierLearners.get(keys[i]).completeTraining();
			System.out.println("Complete");
		}
		System.out.println("All complete!");
	}

	public ClassifierLearner copy(){
		HierarchicalClassifierLearner copy=new HierarchicalClassifierLearner(prototypes);
		String[] keys=toKeyArray(classifierLearners);
		for(int i=0;i<keys.length;i++){
			copy.classifierLearners.put(keys[i],classifierLearners.get(keys[i]).copy());
		}
		return copy;
	}

	public Classifier getClassifier(){
		HashMap<String, Classifier> classifiers=new HashMap<String, Classifier>();
		String[] keys=toKeyArray(classifierLearners);
		for(int i=0;i<keys.length;i++){
			classifiers.put(keys[i],classifierLearners.get(keys[i]).getClassifier());
		}
		return new HierarchicalClassifier(classifiers,prototypes.length);
	}

	public boolean hasNextQuery(){
		return instancePool.hasNext();
		/*
		ClassifierLearner[] learners=toValueArray(classifierLearners);
		for(int i=0;i<learners.length;i++){
			if(learners[i].hasNextQuery()){
				return true;
			}
		}
		return false;
		*/
	}

	public Instance nextQuery(){
		return instancePool.nextInstance();
		/*
		ClassifierLearner[] learners=toValueArray(classifierLearners);
		for(int i=0;i<learners.length;i++){
			if(learners[i].hasNextQuery()){
				return learners[i].nextQuery();
			}
		}
		return null;
		*/
	}

	public void reset(){
		ClassifierLearner[] learners=toValueArray(classifierLearners);
		for(int i=0;i<learners.length;i++){
			learners[i].reset();
		}
	}

	public void setInstancePool(Looper instancePool){
		this.instancePool=instancePool;
		/*
		List list;
		for(list=new ArrayList();instancePool.hasNext();list.add(instancePool.next()));
		ClassifierLearner[] learners=toValueArray(classifierLearners);
		for(int i=0;i<learners.length;i++){
			learners[i].setInstancePool(new Instance.Looper(list));
		}
		*/
	}

	/**
	 * Creates an ExampleSchema (a set of class names) from the given ExampleSchema by
     * finding all the class names in the given schema that start with the given prefix
     * and adding their subclasses (as determined by splitting the class name on ".") 
	 * to the ExampleSchema to be returned.
	 */
	private ExampleSchema createSubSchema(ExampleSchema mainSchema,String prefix,int level){
		String[] labelNames=mainSchema.validClassNames();
		List<String> sublabelNames=new ArrayList<String>();
		for(int i=0;i<labelNames.length;i++){
			if(labelNames[i].startsWith(prefix)){
				sublabelNames.add(labelNames[i].split("\\.")[level]); // this assumes a max hierarchy depth of 2!!
			}
		}
		return new ExampleSchema(sublabelNames.toArray(new String[sublabelNames.size()]));
	}

	public void setSchema(ExampleSchema schema){
		String[] labelNames=schema.validClassNames();
		for(int i=0;i<labelNames.length;i++){
			for(int j=0;j<prototypes.length;j++){
				String prefix=getLabelPrefix(labelNames[i],j);
				if(!classifierLearners.containsKey(prefix)){
					System.out.println("Making new schema and learner for "+prefix);
					ExampleSchema subSchema=createSubSchema(schema,prefix,j);
					ClassifierLearner newLearner;
					if(subSchema.getNumberOfClasses()==1){
						System.out.println("Only 1 class to learn for "+prefix+"; using DummyClassifier and Learner");
						newLearner=new DummyClassifierLearner(subSchema.getClassName(0));
					}
					else{
						newLearner=prototypes[j].copy();
						newLearner.setSchema(subSchema);
					}
					classifierLearners.put(prefix,newLearner);
				}
			}
		}
	}

}
