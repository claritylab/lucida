package edu.cmu.sphinx.decoder.scorer;

import com.amd.aparapi.Device;
import com.amd.aparapi.Kernel;
import static com.amd.aparapi.Kernel.EXECUTION_MODE.CPU;
import static com.amd.aparapi.Kernel.EXECUTION_MODE.GPU;
import static com.amd.aparapi.Kernel.EXECUTION_MODE.JTP;
import static com.amd.aparapi.Kernel.EXECUTION_MODE.SEQ;
import com.amd.aparapi.ProfileInfo;

import com.amd.aparapi.Range;
import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.endpoint.SpeechEndSignal;
import edu.cmu.sphinx.frontend.endpoint.SpeechStartSignal;
import edu.cmu.sphinx.frontend.util.DataUtil;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.GaussianMixture;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.Sphinx3Loader;
import edu.cmu.sphinx.util.props.ConfigurableAdapter;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;
import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Implements some basic scorer functionality, including a simple default
 * acoustic scoring implementation which scores within the current thread, that
 * can be changed by overriding the {@link #doScoring} method.
 *
 * <p>
 * Note that all scores are maintained in LogMath log base.
 *
 * @author Holger Brandl
 */
public class GPUAcousticScorer extends ConfigurableAdapter implements AcousticScorer {

    /**
     * Property the defines the frontend to retrieve features from for scoring
     */
    @S4Component(type = BaseDataProcessor.class)
    public final static String FEATURE_FRONTEND = "frontend";
    protected BaseDataProcessor frontEnd;

    /**
     * An optional post-processor for computed scores that will normalize
     * scores. If not set, no normalization will applied and the token scores
     * will be returned unchanged.
     */
    @S4Component(type = ScoreNormalizer.class, mandatory = false)
    public final static String SCORE_NORMALIZER = "scoreNormalizer";
    private ScoreNormalizer scoreNormalizer;

    private Boolean useSpeechSignals;



    private static int means_size;
    private static int comp_size;
    private static float[] meansArray;
    private static float[] precsArray;
    private static float[] mixWeightArray;
    private static float[] preFactorArray;
    private static int max_senone_size;
    private boolean LoadedModel = false;

    private float[] score_vect;
    private float[] cpu_score_vect;
    private float[] feat_vect = null;

    private Kernel kernel;
            
    List<Double> gpu_times = new ArrayList<Double>();
    List<Double> cpu_times = new ArrayList<Double>();
    List<Double> senone_sizes = new ArrayList<Double>();

    double computeMean(double[] data) {
        double sum = 0.0;
        for (double a : data) {
            sum += a;
        }
        return sum / data.length;
    }

    double computeVariance(double[] data) {
        double mean = computeMean(data);
        double temp = 0;
        for (double a : data) {
            temp += (mean - a) * (mean - a);
        }
        return temp / data.length;
    }

    double computeStdDev(double[] data) {
        return Math.sqrt(computeVariance(data));
    }

    public static double[] convertDoubles(List<Double> doubles) {
        double[] ret = new double[doubles.size()];

        for (int i = 0; i < ret.length; i++) {
            ret[i] = doubles.get(i).doubleValue();
        }

        return ret;
    }

    private static int means_array_size;
    private static int comp_array_size;

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        this.frontEnd = (BaseDataProcessor) ps.getComponent(FEATURE_FRONTEND);
        this.scoreNormalizer = (ScoreNormalizer) ps.getComponent(SCORE_NORMALIZER);
    }

    /**
     * @param frontEnd the frontend to retrieve features from for scoring
     * @param scoreNormalizer optional post-processor for computed scores that
     * will normalize scores. If not set, no normalization will applied and the
     * token scores will be returned unchanged.
     */
    public GPUAcousticScorer(BaseDataProcessor frontEnd, ScoreNormalizer scoreNormalizer) {
        initLogger();
        this.frontEnd = frontEnd;
        this.scoreNormalizer = scoreNormalizer;
    }

    public GPUAcousticScorer() {
    }

    /**
     * Scores the given set of states.
     *
     * @param scoreableList A list containing scoreable objects to be scored
     * @return The best scoring scoreable, or <code>null</code> if there are no
     * more features to score
     */
    public Data calculateScores(List<? extends Scoreable> scoreableList) {
        try {
            Data data;
            while ((data = getNextData()) instanceof Signal) {
                if (data instanceof SpeechEndSignal || data instanceof DataEndSignal) {
                    return data;
                }
            }

            if (data == null || scoreableList.isEmpty()) {
                return null;
            }

            // convert the data to FloatData if not yet done
            if (data instanceof DoubleData) {
                data = DataUtil.DoubleData2FloatData((DoubleData) data);
            }

            //if (feat_vect == null) {
                feat_vect = FloatData.toFloatData(data).getValues();
           // }

            /* float[] featureVector = FloatData.toFloatData(data).getValues();
             PrintWriter pw = new PrintWriter(new FileOutputStream(new File("feat_data.txt"),true));   
             for (int f=0; f < featureVector.length; f++) {          
             pw.printf("%f ", featureVector[f]);
             }   
             pw.printf("\n");
             pw.close();*/
            Scoreable bestToken = doScoring(scoreableList, data);

            // apply optional score normalization
            if (scoreNormalizer != null && bestToken instanceof Token) {
                bestToken = scoreNormalizer.normalize(scoreableList, bestToken);
            }

            return bestToken;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private Data getNextData() {
        Data data = frontEnd.getData();

        // reconfigure the scorer for the coming data stream
        if (data instanceof DataStartSignal) {
            handleDataStartSignal((DataStartSignal) data);
        }
        if (data instanceof DataEndSignal) {
            handleDataEndSignal((DataEndSignal) data);
        }

        return data;
    }

    /**
     * Handles the first element in a feature-stream.
     *
     * @param dataStartSignal
     */
    protected void handleDataStartSignal(DataStartSignal dataStartSignal) {
        Map<String, Object> dataProps = dataStartSignal.getProps();

        useSpeechSignals = dataProps.containsKey(DataStartSignal.SPEECH_TAGGED_FEATURE_STREAM) && (Boolean) dataProps.get(DataStartSignal.SPEECH_TAGGED_FEATURE_STREAM);
    }

    /**
     * Handles the last element in a feature-stream.
     *
     * @param dataEndSignal
     */
    protected void handleDataEndSignal(DataEndSignal dataEndSignal) {
        // we don't treat the end-signal here, but extending classes might do
    }

    public void startRecognition() {
        if (useSpeechSignals == null) {
            Data firstData = getNextData();
            if (firstData == null) {
                return;
            }

            assert firstData instanceof DataStartSignal :
                    "The first element in an sphinx4-feature stream must be a DataStartSignal but was a " + firstData.getClass().getSimpleName();
        }

        if (!useSpeechSignals) {
            return;
        }

        Data data;
        while (!((data = getNextData()) instanceof SpeechStartSignal)) {
            if (data == null) {
                break;
            }
        }
    }

    public void stopRecognition() {
        // nothing needs to be done here

        double gpu_mean = computeMean(convertDoubles(gpu_times));
        double cpu_mean = computeMean(convertDoubles(cpu_times));

        double gpu_stdev = computeStdDev(convertDoubles(gpu_times));
        double cpu_stdev = computeStdDev(convertDoubles(cpu_times));

        System.out.println("GPU mean: " + gpu_mean);
        System.out.println("GPU stdev: " + gpu_stdev);

        System.out.println("CPU mean: " + cpu_mean);
        System.out.println("CPU stdev: " + cpu_stdev);

        System.out.println("GPU speedup (over CPU): " + cpu_mean / gpu_mean);

    }
    
    
    static final float logZero = -Float.MAX_VALUE;
    static final float logBase = 1.0001f;

    static final float naturalLogBase = (float) Math.log(logBase);
    static final float inverseNaturalLogBase = 1.0f / naturalLogBase;

    static final float maxLogValue = (float) (Math.log(Double.MAX_VALUE) * inverseNaturalLogBase);
    static final float minLogValue = (float) (Math.log(Double.MIN_VALUE) * inverseNaturalLogBase);

    private void computeScore2(int gid) {
        
        //System.out.println("Gid: " + gid);
        
       // return;

        //for (int i = 0; i < max_senone_size; i++) {
        score_vect[gid] = logZero;
        for (int j = 0; j < comp_size; j++) {

                // getScore
            // idx = k + D*j + i*W*D
            float logDval = 0.0f;
            for (int k = 0; k < means_size; k++) {
                    //float logDiff = featureVector[k] - mean_trans[k];
                //logDval += logDiff * logDiff * prec_trans[k];
                int idx = k + means_size * j + gid * comp_size * means_size;
                float logDiff = feat_vect[k] - meansArray[idx];
                logDval += logDiff * logDiff * precsArray[idx];
            }
               // System.out.println("NEW comp: " + i + " logDval:"+logDval + " after feature_vect");

                // Convert to the appropriate base.
            //logDval = logMath.lnToLog(logDval);            
            if (logDval != logZero) {
                logDval = logDval * inverseNaturalLogBase;
            }

            int idx2 = j + gid * comp_size;

                // Add the precomputed factor, with the appropriate sign.
            //  logDval -= mixtureComponents[i].getLogPreComputedGaussianFactor();
            logDval -= preFactorArray[idx2];

            /*      if (Float.isNaN(logDval)) {
             System.out.println("gs is Nan, converting to 0");
             logDval = logZero;
             }*/
            if (logDval < logZero) {
                logDval = logZero;
            }
                // end of getScore      
            //          System.out.println("NEW comp: " + i + " logDval:"+logDval + " after getScore, preFactor_array");

            //      float logVal2 = logDval + logMixtureWeights[i];
            float logVal2 = logDval + mixWeightArray[idx2];
        //        System.out.println("NEW comp: " + i + " logVal2:"+logVal2 + " after mixWeightArray");

            //float logVal2 = mixtureComponents[i].getScore(featureVector) + logMixtureWeights[i];
            float logHighestValue = score_vect[gid];
            float logDifference = score_vect[gid] - logVal2;

            // difference is always a positive number
            if (logDifference < 0) {
                logHighestValue = logVal2;
                logDifference = -logDifference;
            }

            //double logInnerSummation = logToLinear(-logDifference);
            float logValue = -logDifference;
            double logInnerSummation;
            if (logValue < minLogValue) {
                logInnerSummation = 0.0;
                //   System.out.println("enter minLogValue");
            } else if (logValue > maxLogValue) {
                logInnerSummation = Double.MAX_VALUE;
                System.out.println("enter maxLogValue");

            } else {
                if (logValue == logZero) {
                    logValue = logZero;
                    System.out.println("enter logValue==LogZero");
                } else {
                    logValue = logValue * naturalLogBase;
                }
                logInnerSummation = Math.exp(logValue);
            }

            logInnerSummation += 1.0;

            //float actual_comp = linearToLog(logInnerSummation);        
            float returnLogValue;
            if (logInnerSummation <= 0.0) {
                returnLogValue = logZero;
                System.out.println("enter logInnerSummation <=0.0");

            } else {
                returnLogValue = (float) (Math.log(logInnerSummation) * inverseNaturalLogBase);
                if (returnLogValue > Float.MAX_VALUE) {
                    returnLogValue = Float.MAX_VALUE;
                    System.out.println("enter returnLogValue > MAX_VALUE");
                } else if (returnLogValue < -Float.MAX_VALUE) {
                    returnLogValue = -Float.MAX_VALUE;
                    System.out.println("enter returnLogValue < -MAX_VALUE");

                }
            }
            // sum log
            score_vect[gid] = logHighestValue + returnLogValue;
            //            System.out.println("NEW comp: " + i + " logTotal:"+logTotal + " after sum_log");
        }
        //return logTotal;
        //}
    }

    /**
     * Scores a a list of <code>Scoreable</code>s given a
     * <code>Data</code>-object.
     *
     * @param scoreableList The list of Scoreables to be scored
     * @param data The <code>Data</code>-object to be used for scoring.
     * @return the best scoring <code>Scoreable</code> or <code>null</code> if
     * the list of scoreables was empty.
     * @throws Exception
     */
    protected <T extends Scoreable> T doScoring(List<T> scoreableList, Data data) throws Exception {

        // original scoring
        Iterator<T> i = scoreableList.iterator();
        T best = i.next();
        best.calculateScore(data);
        while (i.hasNext()) {
            T scoreable = i.next();
            if (scoreable.calculateScore(data) > best.getScore()) {
                best = scoreable;
            }
        }
        
        return best;

        // sequential scoring
        /*long startTime = System.currentTimeMillis();
        this.computeScore_seq();
        long estimatedTime = System.currentTimeMillis() - startTime;
        cpu_times.add((double) estimatedTime);

        System.out.println("CPU: " + estimatedTime);*/

              final int asize = 512;

      /** Input float array for which square values need to be computed. */
      final float[] values = new float[asize];

      /** Initialize input array. */
      for (int a = 0; a < asize; a++) {
         values[a] = a;
      }
      final float[] squares = new float[asize];
      
      //Device.getBest().forEach(asize, id -> squares[id] = (float) (2.0*values[id]));
      
        // kernel scoring
        long startTime = System.currentTimeMillis();

        kernel.put(feat_vect);

        // modes: GPU, CPU, JTP, SEQ
        kernel.setExecutionMode(JTP);
       // kernel.setExecutionMode(SEQ);
        Range r = Range.create(max_senone_size);
        //Range r = Range.create(asize);
        kernel.execute(r);
        long estimatedTime = System.currentTimeMillis() - startTime;
        gpu_times.add((double) estimatedTime);
        System.out.println("Kernel JTP: " + estimatedTime);
       
        kernel.get(score_vect);
        
        //        float local_score[] = new float[senone_size];
        
    /*    int idx2 = 0;
        Iterator<T> i2 = scoreableList.iterator();
        T best = i2.next();     */
                
        //best.calculateScore(data);
        //local_score[idx2] = best.getScore();
        
        // update internal data
      /*  best.setlogAcousticScore(score_vect[idx2]);
        score_vect[idx2] = score_vect[idx2] + best.getLogTotalScore();        
        best.setData(data);
        best.setLogTotalScore(score_vect[idx2]);        
        
        idx2 += 1;
        while (i.hasNext()) {
            T scoreable = i.next();
            scoreable.setlogAcousticScore(score_vect[idx2]);
            score_vect[idx2] = score_vect[idx2] + scoreable.getLogTotalScore();        
            scoreable.setData(data);
            scoreable.setLogTotalScore(score_vect[idx2]);                    
      //      local_score[idx2] = scoreable.calculateScore(data);            

            if (scoreable.getScore() > best.getScore())
                best = scoreable;
            
            idx2 += 1;            
        }*/

      /*  kernel.setExecutionMode(GPU);
        // JTP
        startTime = System.currentTimeMillis();
        kernel.execute(r);
        kernel.get(score_vect);

        estimatedTime = System.currentTimeMillis() - startTime;
        gpu_times.add((double) estimatedTime);
        System.out.println("Kernel GPU: " + estimatedTime);*/
        
       /* float best_score_kernel = -Float.MAX_VALUE;
        for (int k = 0; k < score_vect.length; k++) {
            if (score_vect[k] > best_score_kernel) 
                best_score_kernel = score_vect[k];
        }
        float best_score_cpu = best.getScore();*/
                
        // Report target execution mode: GPU or JTP (Java Thread Pool).
        System.out.println("Execution mode = " + kernel.getExecutionMode());
        
     //   System.out.println("Best score kernel: " + best_score_kernel);
       // System.out.println("Best score CPU: " + best_score_cpu);
      
        return best;
    }

    // Even if we don't do any meaningful allocation here, we implement the methods because
    // most extending scorers do need them either.
    public void allocate() {

        if (LoadedModel == false) {

            LoadedModel = true;

            max_senone_size = Sphinx3Loader.senonePool.size();
            GaussianMixture gm_tmp = (GaussianMixture) Sphinx3Loader.senonePool.get(0);
            comp_size = (int) (gm_tmp).getMixtureComponents().length;
            means_size = gm_tmp.getMixtureComponents()[0].getMeanTransformed().length;

            score_vect = new float[max_senone_size];
            
            //  System.out.println("Means_size: " + means_size);
            // allocate means_trans data        
            means_array_size = max_senone_size * comp_size * means_size;
            comp_array_size = max_senone_size * comp_size;
            meansArray = new float[means_array_size];
            precsArray = new float[means_array_size];
            mixWeightArray = new float[comp_array_size];
            preFactorArray = new float[comp_array_size];
            int idx = 0;
            int idx2 = 0;
            for (int i = 0; i < max_senone_size; i++) {
                GaussianMixture gm = (GaussianMixture) Sphinx3Loader.senonePool.get(i);
                float[] weights = gm.getLogMixtureWeights();
                for (int j = 0; j < comp_size; j++) {
                    float[] means = gm.getMixtureComponents()[j].getMeanTransformed();
                    float[] precs = gm.getMixtureComponents()[j].getPrecisionTransformed();
                    mixWeightArray[idx2] = weights[j];
                    preFactorArray[idx2] = gm.getMixtureComponents()[j].getLogPreComputedGaussianFactor();
                    idx2 = idx2 + 1;
                    for (int k = 0; k < means_size; k++) {
                        //int idx = x + comp_size * (j + means_size * k);
                        meansArray[idx] = means[k];
                        precsArray[idx] = precs[k];
                        idx = idx + 1;
                    }
                }
            }
            
            kernel = new Kernel() {
            @Override
            public void run() {
                int id = getGlobalId();
                computeScore2(id);
                //squares[id] = values[id] * values[id];
               }
            };
            
            kernel.setExplicit(true);
            kernel.put(score_vect);
            kernel.put(mixWeightArray);
            kernel.put(preFactorArray);
            kernel.put(meansArray);
            kernel.put(precsArray);           
        }

    }

    public void deallocate() {
    }
}
