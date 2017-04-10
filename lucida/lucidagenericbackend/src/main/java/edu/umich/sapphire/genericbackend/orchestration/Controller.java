package edu.umich.sapphire.genericbackend.orchestration;

/**
 * Since there are at least interfaces for Thrift and REST, there needs to be a controller behind the servers to
 * manage different pipelines. This is it.
 */
public class Controller {
    private Pipeline qaPipe = new PipelineImplementation();

    public Pipeline getQaPipe() {
        return qaPipe;
    }

    public void setQaPipe(Pipeline qaPipe) {
        this.qaPipe = qaPipe;
    }

}
