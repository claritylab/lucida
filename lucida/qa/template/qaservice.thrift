# .thrift file is an interface that describes the services
# provided to the client.
# When the thrift compiler runs, it produces client/server stubs.
# Clients include these files to correctly serialize args.
# Servers include these files to correctly serialize return values.
namespace java qastubs
namespace cpp qastubs

service QAService
{
  string askFactoidThrift(1:string question),

  list<string> askListThrift(1:string question)
}
