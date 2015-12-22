include "types.thrift"

namespace java edu.umich.clarity.thrift

service SchedulerService {
	void registerBackend(1: types.RegMessage message),
	// void enqueueFinishedQuery(1: types.QuerySpec query)
	types.THostPort consultAddress(1: string serviceType)
	void updateLatencyStat(1: string name, 2: types.LatencyStat latencyStat);
}

service IPAService {
	// void updatBudget(1: double budget),
	binary submitQuery(1: types.QuerySpec query)
}