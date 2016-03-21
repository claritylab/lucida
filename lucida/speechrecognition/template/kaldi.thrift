//Kaldi Thrift 

service KaldiService {
	string kaldi_asr(1: string audio_file);
}
