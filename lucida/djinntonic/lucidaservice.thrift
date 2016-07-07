include "lucidatypes.thrift"

service LucidaService {
    // create an intelligent instance based on supplied LUCID
    void create(1:string LUCID, 2:lucidatypes.QuerySpec spec);

    // tell the intelligent instance to learn based on data supplied in the query
    void learn(1:string LUCID, 2:lucidatypes.QuerySpec knowledge);

    // ask the intelligence to infer using the data supplied in the query
    string infer(1:string LUCID, 2:lucidatypes.QuerySpec query);
}