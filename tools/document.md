# Brief spec for gui backend service

### Structure

There are two collections stored in MongoDB for the use of service registry.

- `service_info`: store information for registered micro services.
- `workflow_info`: store information for designed workflow information.

With this design, Lucida serves as a platform to adminstrate micro services that plug into the Lucida platform. There are two key components for the design: service and workflow. Service is the basic unit for query processing, and workflow specifies query transferring path to trigger needed services.

##### `service_info`

Each document stored inside represents one micro service. Each document is a BSON object with the following format:

```
post = {
	"name": NAME_OF_SERVICE, # name of service
	"acronym": ACRONYM_OF_SERVICE_NAME, # acronym of service
	"num": NUM_OF_INSTANCE, # number of instance
	"host_port": [{
		"host": HOST0, 
		"port": PORT0
	}, {
		"host": HOST1, 
		"port": PORT1
	}], # list of dict for host/port pair of instances
	"input": INPUT_TYPE, # input type
	"learn": LEARN_TYPE, # learn type
	"location": LOCATION # location of service in local
}
```

##### `workflow_info`

Each document stored inside represents one workflow. Each document is a BSON object with the following format:

```
post = {
	"name": NAME_OF_SERVICE, # name of workflow
	"input": INPUT_TYPE, # allowed input type
	"classifier": CLASSIFIER_DATA_PATH, # classifier data path
	"code": WORKFLOW_IMPLEMENTATION_CODE # code for implementation of the workflow
}
```