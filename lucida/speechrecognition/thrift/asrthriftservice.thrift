service ASRThriftService {
    bool conf(1:string message)
    void context(1:string cntxt)
    void start()
    void push(1:binary data)
    void stop()
    void abort()
}
