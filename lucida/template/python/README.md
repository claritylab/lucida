# Template microservice in Python

This is a template of microservice built in lucida. To build your own service, follow the steps below.

## Major Dependencies

- None

# Structure

- `server/`: implementation of the template server
- `client/`: implementation of the template testing client

### Step 0: move the directory 

To get started, place the directory under [`lucida/lucida`](../../) folder, and change the name of your directory into a specified name represent your service.

### Step 1: change the configuration

Change the port number for your service (default is 8887) in [`TemplateConfig.py`](TemplateConfig.py).

### Step 2: implement your own create/learn/infer methods

Implement your own create/learn/infer methods in [`server/server.py`](server/server.py). The spec of these three function is in the top-level [`readme`](../../../README.md). Your are free to import different packages for your service, but remember to add the dependence correctly.

### Step 3: update the `Makefile`

Update the [`Makefile`](Makefile). The default one has included the generating Thrift stubs code. You only need to add the dependencies of your own service.

### Step 4: test your service individually

Change the [test application](client) corresponding to your service. After that, do the following steps under this directory to test your service. Remember to change the test query to make sure your service works.

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

### Step 5: insert your service into Lucida

Modify the [`tools/start_all_tmux.sh`](../../../tools/start_all_tmux.sh) and [`lucida/Makefile`](../../Makefile) so that `make local` and `make start_all` include your service.

