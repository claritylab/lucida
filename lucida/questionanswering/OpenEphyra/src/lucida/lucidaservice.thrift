include "lucidatypes.thrift"

service LucidaService {
    // create an intelligent instance based on supplied LUCID
    void create(1:string LUCID, 2:lucidatypes.QuerySpec spec) throws (1:lucidatypes.RuntimeException e);

    // tell the intelligent instance to learn based on data supplied in the query
    void learn(1:string LUCID, 2:lucidatypes.QuerySpec knowledge) throws (1:lucidatypes.RuntimeException e);

    // ask the intelligence to infer using the data supplied in the query
    string infer(1:string LUCID, 2:lucidatypes.QuerySpec query) throws (1:lucidatypes.RuntimeException e);

    void ping();
}
