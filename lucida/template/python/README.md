# Template microservice in Python

This is a template of microservice built in lucida. To build your own service, follow the steps below.

## Major Dependencies

- None

# Structure

- `server/`: implementation of the template server
- `client/`: implementation of the template testing client

### Step 0: design your service workflow

To get started, first design your service workflow. You can create the workflow that includes the services already in Lucida. After designing your workflow, modify the configuration file [`lucida/commandcenter/controller/Config.py`](../../commandcenter/controllers/Config.py). See the top-level [`README.md`](../../../README.md) for details.

### Step 1: move the directory 

Place the directory under [`lucida/lucida`](../../) folder, and change the name of your directory into a specified name represent your service.

### Step 2: change the configuration

Add the port information for your service in [`config.properties`](../../config.properties) with this format.
```
<NAME_OF_YOUR_SERVICE>_PORT=<YOUR_SERVICE_PORT_NUMBER>
```

### Step 3: implement your own create/learn/infer methods

Implement your own create/learn/infer methods in [`server/server.py`](server/server.py). The spec of these three function is in the top-level [`README.md`](../../../README.md). Your are free to import different packages for your service, but remember to add the dependencies correctly.

### Step 4: update the `Makefile`

Update the [`Makefile`](Makefile). The default one has included the generating Thrift stubs code. You only need to add the dependencies of your own service.

### Step 5: test your service individually

Change the [test application](client/client.py) to fit with your service. Remember to change the test query to make sure your service really works. After that, do the following steps under this directory to test your service.

- build 

 ```
 make all
 ```

- start server

 ```
 make start_server
 ```
 
- start testing

 ```
 make start_test
 ```

### Step 6: insert your service into Lucida

Once the test of your service passes, you can insert it into Lucida. Modify the [`tools/start_all_tmux.sh`](../../../tools/start_all_tmux.sh) and [`lucida/Makefile`](../../Makefile) so that `make local` and `make start_all` include your service.

### Step 7: add training data for your own query class

Define a custom type of query that your service can handle and create the following file in the [`lucida/commandcenter/data/`](../../commandcenter/data/) directory:

```
class_<NAME_OF_YOUR_QUERY_CLASS>.txt
```

, and have at least 40 pieces of text in it, each being one way to ask about the same question.
