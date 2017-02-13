package edu.umich.sapphire.genericbackend.orchestration;


/**
 * The pipeline is static in assuming the same stage ordering without branching
 * If more complex pipelines should ever be investigated, consider the Apache Commons Pipeline workflow framework
 * or the Chain of Responsibility pattern (cmp. GoF)
 */
public class PipelineImplementation extends Pipeline {

    public void PipelineQa() {}

    @Override
    public String ask(String question) {

        // Calculate result from question - here placeholder (prepend "You said: ")
        String result = "You said: " + question;

        return result;
    }
}
