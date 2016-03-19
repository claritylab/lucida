# This file outlines the command center's API.
# Thrift will generate stubs that handle serialization/deserialization
# during remote procedure calls.
# Services register with the command center using the registerService() function.

namespace cpp cmdcenterstubs
namespace java cmdcenterstubs

# MachineData is the information that services send
# when they wish to register with the command center.
struct MachineData
{
	1:string name,
	2:i32 port
}

struct QueryData
{
	1:string audioData = "",
	2:string audioFormat = "",
	3:bool audioB64Encoding = false,
	4:string imgData = "",
	5:string imgFormat = "",
	6:bool imgB64Encoding = false,
	7:string textData = ""
}

service CommandCenter
{
  # service <--> command center API
  void registerService(1:string serviceType, 2:MachineData mDataObj)

  # command center <--> client API
  string handleRequest(1:QueryData data)

  # simple function to test connections
  void ping()
}
