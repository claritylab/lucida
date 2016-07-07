# Lucida

Lucida is a speech and vision based intelligent personal assistant inspired by
[Sirius](http://sirius.clarity-lab.org). Visit the provided readmes in
[lucida](lucida) for instructions to build Lucida and follow the instructions to
build [lucida-suite here](http://sirius.clarity-lab.org/sirius-suite/).  Post to
[Lucida-users](http://groups.google.com/forum/#!forum/sirius-users) for more
information and answers to questions. The project is released under [BSD
license](LICENSE), except certain submodules contain their own specific
licensing information. We would love to have your help on improving Lucida, and
see [CONTRIBUTING](CONTRIBUTING.md) for more details.

## Overview

- `lucida`: back-end services and command center (CMD). 
Currently, there are 4 categories of back-end services:
speech recognition (ASR), image matching (IMM), question answering (QA),
and calendar (CA). By default, Lucida uses the following ports:
3000, 8080 for CMD; 8888 for ASR (web socket listener as part of CMD) ; 
8082 for IMM; 8083 for QA; 8084 for CA.

- `tools`: dependencies necessary for compiling Lucida.
Due to the fact that services share some common dependencies like Thrift,
all services should be compiled after these dependencies are installed.

## Lucida Local Development

- From this directory, type: `make local`. This will run scripts in `tools/` to
  install all the required depedencies. After that, it compiles back-end services
  in `lucida/`. Note: if you would like to install the
  packages locally, each install script must be modified accordingly. This will
  also build `lucida-suite` and `lucida`.
- Similar to what is set in the Makefile, you must set a few environment
  variables. From the top directory:
```
export LD_LIBRARY_PATH=/usr/local/lib
export LUCIDAROOT=`pwd`/lucida
```
- Start all the services:
```
make start_all
```
- To test, open your browser, and go to `http://localhost:3000/`.

## Lucida Docker Deployment

- Install Docker: refer to
  [https://docs.docker.com/engine/installation/](https://docs.docker.com/engine/installation/)
- Install Docker Compose: use `pip install docker-compose` or refer to
  [https://docs.docker.com/compose/install/](https://docs.docker.com/compose/install/)
- Pull the Lucida image. There are several available:  
`docker pull claritylab/lucida:latest # add your own facts` (TODO!!!!!)
- Pull the speech recognition image (based on
  [kaldi-gstreamer-server](https://github.com/alumae/kaldi-gstreamer-server)):  
`docker pull claritylab/lucida-asr`
- From the top directory of Lucida:  
`docker-compose up`
- In Chrome, navigate to `localhost:8081`

Note: Instructions to download and build Sirius can be found at
[http://sirius.clarity-lab.org](http://sirius.clarity-lab.org)

## Design Notes -- How to Add Your Own Service into Lucida?

### Start with Thrift

Thrift is an RPC framework with the advantages of being efficient and language-neutral. 
It was originally developed by Facebook and now developed by both the open-source community (Apache Thrift) and Facebook.
We use both Apache Thrift and Facebook Thrift because Facebook Thrift has a fully asynchronous C++ server but does not support
Java very well. Also, Apache Thrift seems to be more popular.
Therefore, we use Apache Thrift for services written in Python and Java,
and Facebook Thrift for services written in C++.

One disadvantage about Thrift is that the interface has to be pre-defined and implemented by each service. 
If the interface changes, all services have to re-implement the interface. 
We try to avoid changing the interface by careful design, but if you really need to adapt the interface for your need,
feel free to modify, but make sure that all services implement and use the new interface.

### Does it mean I only need to implement the Thrift interface?

The short answer is no, but you only need to configure the command center (CMD) besides implementing the Thrift interface
in order to add your own service into Lucida. Let's break it down into two steps:

1. Implement the Thrift interface jointly defined in `lucida/lucidaservice.thrift` and `lucida/lucidatypes.thrift`.

  1. `lucida/lucidaservice.thrift`:

    ```
    include "lucidatypes.thrift"
    service LucidaService {
       void create(1:string LUCID, 2:lucidatypes.QuerySpec spec);
       void learn(1:string LUCID, 2:lucidatypes.QuerySpec knowledge);
       string infer(1:string LUCID, 2:lucidatypes.QuerySpec query);
    }
    ```
    
    The basic funtionalities that your service needs to provide are called `create`, `learn`, and `infer`. 
    They all take in the same type of parameters, a `string` representing the Lucida user ID (`LUCID`),
    and a custom type called `QuerySpec` defined in `lucida/lucidatypes.thrift`. 
    The command center invokes these three procedures implemented by your service,
    and services can also invoke these procedures on each other to achieve communication.
    Thus the typical data flow looks like this:
    
    ```Command Center (CMD) -> Your Own Service (YOS)```
    
    But it also can be like this:
    
    ```Command Center (CMD) -> Your Own Service 1 (YOS1) -> Your Own Service 2 (YOS2) -> Your Own Service 3 (YOS3)```
    
    In this scenario, the command center sends a request to YOS1, YOS1 processes the query
    and sends the reqeust to YOS2. 
    
    Make sure to implement the asynchronous Thrift interface.
    If YOS1 implements the asynchronous Thrift interface, which it should,
    it won't block on waiting for the response from YOS2. Rather, a callback function is registered
    and the current thread can resume execution, so that when the reponse from YOS2 gets back to YOS1, the callback
    function is executed, in which YOS1 can either further process the response or immediately return it to CMD.
    If YOS1 implements the synchronous Thrift interface, the current thread cannot continue execution until
    YOS2 returns the response, so the operating system will suspend the current thread and perform a thread context switch
    which incurs overhead. The current thread sleeps until YOS2 returns. 
    Hopefully you see why we prefer asynchronous implementation of the thrift interface to synchronous implementation.
    See (3) of step 1 for details.
    
    `create`: create an intelligent instance based on supplied LUCID
    
    `learn`: tell the intelligent instance to learn based on data supplied in the query. 
    Although it must be implemented, you can choose to do nothing in the function body or simply print some message
    if your service cannot learn new knowledge. For example, it may be hard to retrain a DNN model, so the facial recognition
    service simply print a message when it receives a learn request. You can tell the command center not to send a learn request
    to your service, which will be explained soon.
    If your service can handle new knowledge in the form of either image or text, you should tell the command center, which will be explained soon.
    
    `infer`: ask the intelligence to infer using the data supplied in the query.
    This is the most important functionality in the sense that it receives a query in the form of either image or text and
    returns the response in the form of a string. As will be explained soon, a string can be either plain text or image data,
    but usually human readable plain text is returned and this is what is assumed in the command center.

  2.  `lucida/lucidatypes.thrift`:

    ```
    struct QueryInput {
        1: string type;
        2: list<string> data;
        3: list<string> tags;
    }
    struct QuerySpec {
        1: string name;
        2: list<QueryInput> content;
    }
    ```
    
    A `QuerySpec` has a name, which is `create` for `create`, `knowledge` for `learn`, and `query` for `infer`. 
    A `QuerySpec` also has a list of `QueryInput` which is the data payload. 
    A `QueryInput` consists of a `type`, a list of `data`, and a list of `tags`. 
    
    * If the function call is `learn`:
    
    Only one `QueryInput` is sent to your service currently, but you shouldn't assume this. Instead,
    you should iterate through all `QueryInput`s and grab all data to learn.
    The `type` can be `text` for plain text, `url` for url address, `image` for image,
    or `unlearn` (undo learn) for the reverse process of learn.
    A service can handle either text or image, and if it can handle text, the type will be
    `text`, `url`, or `unlearn`, 
    and if it can handle image, the type will be `image` or `unlearn`.
    The command center guarantees that a service that can only receive text won't receive image
    as long as the command center is configured correctly.
    See step 2 for more details about this and how to define your own types (e.g. `video`).
    If `type` is `text` or `url`, `data[i]` is the `i`th piece of text as new knowledge
    and `tags[i]` is the id of the `i`th piece of text generated by a hash function in the command center;
    if `type` is `image`, `data[i]` is the `i`th image as new knowledge
    (notice that it is the actual string representation of an image and thus can very long),
    and `tags[i]` is the label/name of the `i`th image received from the front end;
    if `type` is `unlearn`, `data` should be ignored by your service (usually a list of an empty string),
    and `tags[i]` is the id of the text to delete or the label of the image to delete
    based on whether the service can handle text or image.
    See step 2 for more details on how to specify the type of knowledge that your service can handle.
    
    * If the function call is `infer`:
    
    Each `QueryInput` in `content` corresponds to one service (CMD is not considered to be a service)
    in the service graph, a connected directed acyclic graph (DAG) describing all services that are needed for the query.
    Thus, for the following service graph, only one `QueryInput` is generated by the command center`:
    
    ```Command Center (CMD) -> Your Own Service (YOS)```
    
    The `type` can be `text` for plain text, or `image` for image. 
    Similar to `learn`, if `type` is `text` or `url`, `data[i]` is the `i`th piece of text as query;
    if `type` is `image`, `data[i]` is the `i`th image as query.
    However, `tags` have the following format:
    
    ```
    [host, port, <size of the following list>, <list of integers>]
    ```
    
    , which indicates a node in the service graph. The `host:port` specifies the location of the service,
    and second list specifies the indices of nodes that the service points to.
    The size matches the length of the list, and thus must be a non-negative integer (0 indicating an empty list).
    If the list is empty, the node does not send any further request.
    
    Therefore, the above service graph results in a `QuerySpec` like this:
    
    ```
    { name: "query", 
    content: [ 
    { type: "text",
    data: [ "What's the speed of light?" ],
    tags: ["localhost", "8083", "0"] }
    ] }
    ```
    
    . We can define arbitrarily complicated service graphs. For example, for the following service graph:
    
    ![Alt text](service_graph_0.png?raw=true "Service Graph")
    
    , the resulting `QuerySpec` may look like this, assuming YOSX is running at 909X:
    
    ```
    { name: "query", 
    content: [
    { type: "image",
    data: [ "1234567...abcdefg" ],
    tags: ["localhost", "9090", "1", 2"] },
    { type: "image",
    data: [ "1234567...abcdefg" ],
    tags: ["localhost", "9091", "2", "2", "3"] },
    { type: "text",
    data: [ "Which person in my family is in this image?" ],
    tags: ["localhost", "9092", "0"] },
    { type: "image",
    data: [ "1234567...abcdefg" ],
    tags: ["localhost", "9093", "0"] }
    ] }
    ```
    
    . Notice that if the order of `QueryInput` in `content` is rearranged, the `QuerySpec` still corresponds to the same graph.
    In fact, there are `2^(N)` valid `QuerySpec`s for a given graph, and you need to define only one of them in
    the configuration file of the command center. Notice that the starting nodes, YOS0 and YOS1, need to be specified separately,
    so that the command center knows where to send the request to.
    See more detais in step 2.
    
    The command center guarantees to send a valid `QuerySpec` as long as the user defines the service graph correctly,
    but your service is responsible for parsing the graph, sending the request(s) to the service(s) it points to,
    and returning the response(s) to the service(s) it is pointed to by.
    Suppose in the above example, YOS0 does not send the request to YOS2,
    and YOS2 is written in a way that it must process requests from both YOS0 and YOS1 before returning to YOS1.
    Then YOS2 cannot make progress, which leads to YOS1 waiting for YOS2, which leads to the command center waiting for YOS1.
    Each service is also allowed to ignore or modify the graph if that is necessary, but that should be done with caution.
    
    Although the service graph can be very complicated, it is usually very simple. At least for the current Lucida services,
    the most complicated graph looks like this:
    
    ```Command Center (CMD) -> IMM -> QA```
    
    . Thus, most current services can ignore the `tags` without any problem.

  3. Here are the concrete code examples that you can use for your own service:

    If it is written in C++, refer to the code in `lucida/lucida/imagematching/opencv_imm/server/`, especially `IMMHandler.h`,
    `IMMHandler.cpp`, and `IMMServer.cpp`.
    
    If it is written in Java, refer to the code in `lucida/lucida/calendar/src/main/java/calendar/`, especially `CAServiceHandler.java` and `CalendarDaemon.java`.
    
  4. Here is a list of what you need to do for step 1: 
    
    * Add a thrift wrapper which typically consists of a Thrift handler
      which implements the Thrift interface described above, and a server daemon which is the entry point of your service.
      Refer to the sample code mentioned above.
    
    * Modify your Makefile for compiling your service and shell script for starting your service.
    
    * Test your service.
    
    * Put your service into a Docker image, and add Kubernetes `yaml` scripts for your service.

2. Confugure the command center. 

  [This](lucida/commandcenter/controllers/Config.py) is the only file you need to modify,
  but you may also need to add sample queries to [this directory](lucida/commandcenter/data/)
  as training data for the query classifier.
  
  1. Modify the configuration file.
    
    ```
    SERVICES = { 
    	'IMM' : Service('IMM', 8082, 'image', 'image'),  # image matching
    	'QA' : Service('QA', 8083, 'text', 'text'), # question answering
    	'CA' : Service('CA', 8084, 'text', None), # calendar
    	}
    
    CLASSIFIER_DESCRIPTIONS = { 
    	'text' : { 'class_QA' :  Graph([Node('QA')]),
    	           'class_CA' : Graph([Node('CA')]) },
    	'image' : { 'class_IMM' : Graph([Node('IMM')]) },
    	'text_image' : { 'class_QA': Graph([Node('QA')]),
    					         'class_IMM' : Graph([Node('IMM')]), 
    					         'class_IMM_QA' : Graph([Node('IMM', [1]), Node('QA')]) } }
    ```
    
    `SERVICES` is a dictionary from service name to service object.
    A service object is constructed this way:
    
    ```Service(name, port, input_type, learn_type)```
    
    , where `name` is the name of the service, `port` is the port number,
    `input_type` is the type of input that the service can handle which is either `text` or `image`,
    and `learn_type` is the type of knowledge that the service can receive which is either`text`, `image`, or `None`.
    
    Notice you do not need to specify the IP address of the service. By default it is `localhost`,
    but if you use Kubernetes and run your service behind a Kubernetes `Service`,
    Kubernetes dynamically assigns an IP address for the service and sets an environment variable
    `<SERVICE_NAME>_PORT_<PORT>_TCP_ADDR` for each running container in the Kubernetes cluster.
    This implies that the Kubernetes service must have the same name as the service name defined in this file.
    For example, you have to define a Kubernetes service called `IMM` and run your container behind it,
    so that the command center has the following environment variable set:
    
    ```
    IMM_PORT_8082_TCP_ADDR
    ```
    
    . Use your service name consistently. Do not use `IMM` here but `image_matching` in the Kubernetes `yaml` file.
    See more details on how to define Kubernetes service at [tools/deploy/](tools/deploy/)
    
    Notice that you do not to list `ASR` (automatic speech recognition) service here.
    The reason is that we currently use [kaldi gstremer server] (https://github.com/alumae/kaldi-gstreamer-server)
    which receives real-time voice query from front end JavaScript rather than the command center.
    However, if you want to redirect the voice query to the command center and then send it to your own
    ASR service, you also need to modify [the Javascript front end] (lucida/commandcenter/static/js/).
  
    `CLASSIFIER_DESCRIPTIONS` is a dictionary from query class to service graph.
    Internally, the command center uses a query classifier that predicts the services that are needed
    from the user input query. In the above example, `class_QA` corresponds to a list of
    sample questions that a user can possibly ask about generic QA style questions
    which is defined in `lucida/commandcenter/data/class_QA.txt`. Thus, if you simply want to replace the current
    QA implementation with your own, you can still use the training data, and only modify the service graph that
    `class_QA` maps to like this:
    
    ```
    'text_image' : { 'class_QA': Graph([Node('NAME_OF_MY_OWN_QA_SERVICE')]), ...
    ```
    
    However, if you need to define another set of questions that a user can ask, e.g. questions about facial recognition,
    you should read the next section on how to add training data.
    
    Each query class maps to a service graph described in step 1. Notice a service `Graph` object is constructed
    with a list of `Node`. Each `Node` is constructed with the servide name and an optional list of node indices that
    the current node points to. If not provided, it is an empty list, meaning the node does not point to any other node.
    For example, `'class_IMM_QA' : Graph([Node('IMM', [1]), Node('QA')])` is represented as:
    
    ```
    IMM -> QA
    ```
    
    Notice that we do not define which nodes the current node is pointed to by, so we do not know which node is pointed to
    by the command center which is not a service node. Thus, we need to specify the starting nodes separately.
    This is done by an optional second paramater in the constructor of `Graph`:
    
    ```
    def __init__(self, node_list, starting_indices=[0]):
    ```
    
    As you see, by default, the command center assumes the 0th node in the node list is the starting node.
    Thus, the command center will create a `QuerySpec` like the following and then it to the `IMM` service:
    
    ```
    { name: "query", 
    content: [
    { type: "image",
    data: [ "1234567...abcdefg" ],
    tags: ["localhost", "8082", "1", 1"] },
    { type: "text",
    data: [ "How old is this person?" ],
    tags: ["localhost", "8083", "0"] }
    ] }
    ```
    
    The `IMM` service receives this `QuerySpec` along with the user ID, and is responsible for further sending
    the request to the `QA` service. The `IMM` service is allowed to modify the `QuerySpec` and possibly
    send a reconstructed `QuerySpec` to `QA`, but as long as `IMM` finally returns a response to the
    command center, it is fine. This degree of flexibility opens up opportunities for complicated communication
    between your services. Usually, a simple graph one node suffices, because that one node may be an entry point
    to a cluster of your own services, which have their own way of communication (which may not be Thrift!).
    In other words, as long as your service cluster exposes one node to the command center through Thrift,
    it is considered to be a Lucida service!
    
    Be aware that there are other parameters that you can change in this configuration file,
    which are self-explanatory in the file.
    
  2. Add training data for your own query class.
  
    We already prepare some sample training data in `lucida/commandcenter/data/`, but if you need to define
    a custom type of query that your service can handle, you should create the following file in the above directory:
  
    ```
    class_<NAME_OF_YOUR_QUERY_CLASS>.txt
    ```
    
    , and fill at least 40 pieces of text in it, each being one or several sentences asking about that query class.
    
