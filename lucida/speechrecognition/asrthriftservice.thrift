service ASRThriftService {
    void request_id(1:string id)
    void user(1:string user)
    void context(1:string cntxt)
    void start()
    void push(1:binary data)
    void stop()
    void abort()
}
