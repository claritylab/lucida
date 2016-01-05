# .thrift file is an interface that describes the services
# provided to the client.
# When the thrift compiler runs, it produces client/server stubs.
# Clients include these files to correctly serialize args.
# Servers include these files to correctly serialize return values.
namespace java parserstubs

typedef i32 int // We can use typedef to get pretty names for the types we are using

service ParserService
{
  bool parseThrift(1:string Indri_path, 2:list<string> urls, 3:list<string> files),

  void ping()
}
