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

service FileTransferSvc {
    string ping(),
    void send_file(1: QueryData data, 2: string uuid),
    string get_response(1: string uuid),
}
