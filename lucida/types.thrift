namespace java edu.umich.clarity.thrift

struct THostPort {
	1: string ip;
	2: i32 port;
}

struct QueryInput {
	1: string type;				//type of data (i.e. audio, image, etc)
	2: list<string> data;
	3: optional list<string> tags;
}

struct QuerySpec {
	1: optional string name;
	2: list<QueryInput> content;
}

struct RegMessage {
	1: string app_name;
	2: THostPort endpoint;
}

struct LatencyStat {
	1: THostPort hostport;
	2: i64 latency;
}