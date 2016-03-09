// This handler implements the services provided to the client.

// Open Ephyra packages
import info.ephyra.OpenEphyra;
import info.ephyra.search.Result;
import info.ephyra.io.MsgPrinter;

// Java packages
import java.util.List;
import java.util.ArrayList;

// Interface definition
import qastubs.QAService;

/** Implementation of the question-answer interface defined
 * in the question-answer thrift file. A client request to any
 * method defined in the thrift file is handled by the
 * corresponding method here.
 */
public class QAServiceHandler implements QAService.Iface {
  /** An object that lets the question-answer wrapper use
   * the end-to-end OpenEphyra framework.
   */
  private OpenEphyra oe;
  private String defaultAnswer;

  /** Constructs the handler and initializes its OpenEphyra
   * object.
   */
  public QAServiceHandler() {
    String dir = "";

    MsgPrinter.enableStatusMsgs(true);
    MsgPrinter.enableErrorMsgs(true);

    oe = new OpenEphyra(dir);
    defaultAnswer = "Factoid not found in knowledge base.";
  }

  /** Forwards the client's question to the OpenEphyra object's askFactoid
   * method and collects the response.
   * @param question eg. "what is the speed of light?"
   */
  public String askFactoidThrift(String question) {
    MsgPrinter.printStatusMsg("askFactoidThrift(): Arg = " + question);
    
    Result result = oe.askFactoid(question);
    String answer = defaultAnswer;
    if (result != null) {
      answer = result.getAnswer();
    }

    return answer;
  }

  /** Forwards the client's question to the OpenEphyra object's askList
   * method and collects the response.
   * @param question eg. "name the US Presidents"
   */
  public List<String> askListThrift(String question) {
    float relThresh = 0.5f; //user may change this value
    
    MsgPrinter.printStatusMsg("askListThrift(): Arg = " + question);

    Result[] results = oe.askList(question, relThresh);
    List<String> answersList = new ArrayList<String>();
    if (results.length > 0) {
      // add all answers to answersList
      for (Result r : results) {
        answersList.add(r.getAnswer());
      }
    } else {
      answersList.add(defaultAnswer);
    }
    return answersList;
  }

  public void ping() {
    System.out.println("pinged");
  }
}

